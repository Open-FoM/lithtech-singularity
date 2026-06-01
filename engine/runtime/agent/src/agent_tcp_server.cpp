#include "agent_tcp_server.h"

#include <chrono>
#include <cstring>
#include <future>
#include <optional>
#include <string>
#include <string_view>

#include "agent_protocol.h"
#include "agent_queue.h"
#include "agent_tool.h"

// ---- Platform socket layer ------------------------------------------------ //
#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
namespace {
using socket_t = SOCKET;
constexpr socket_t kInvalidSocket = INVALID_SOCKET;
int close_socket(socket_t s) { return ::closesocket(s); }
// One-time Winsock init for the lifetime of the process.
struct WinsockInit {
  WinsockInit() {
    WSADATA data;
    WSAStartup(MAKEWORD(2, 2), &data);
  }
  ~WinsockInit() { WSACleanup(); }
};
const WinsockInit g_winsock_init;
} // namespace
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>
namespace {
using socket_t = int;
constexpr socket_t kInvalidSocket = -1;
int close_socket(socket_t s) { return ::close(s); }
} // namespace
#endif

namespace ltjs::agent {

namespace {

constexpr std::size_t kMaxLineBytes = 1u << 20; // 1 MiB guard against runaway input.

socket_t ToSocket(std::intptr_t fd) { return static_cast<socket_t>(fd); }
std::intptr_t FromSocket(socket_t s) { return static_cast<std::intptr_t>(s); }

// Block up to timeout_ms for the socket to become readable. Returns true if
// readable, false on timeout or error.
bool WaitReadable(socket_t s, int timeout_ms) {
  fd_set read_set;
  FD_ZERO(&read_set);
  FD_SET(s, &read_set);
  timeval tv;
  tv.tv_sec = timeout_ms / 1000;
  tv.tv_usec = (timeout_ms % 1000) * 1000;
  const int rc = ::select(static_cast<int>(s) + 1, &read_set, nullptr, nullptr, &tv);
  return rc > 0 && FD_ISSET(s, &read_set);
}

bool SendAll(socket_t s, std::string_view data) {
  std::size_t sent = 0;
  while (sent < data.size()) {
    const int n = static_cast<int>(::send(s, data.data() + sent, data.size() - sent, 0));
    if (n <= 0) {
      return false;
    }
    sent += static_cast<std::size_t>(n);
  }
  return true;
}

} // namespace

AgentControlServer::AgentControlServer(AgentCommandQueue &queue, Config config)
    : queue_(queue), config_(std::move(config)) {}

AgentControlServer::~AgentControlServer() { Stop(); }

bool AgentControlServer::Start() {
  if (running_.load(std::memory_order_acquire)) {
    return true;
  }

  const socket_t listen_sock = ::socket(AF_INET, SOCK_STREAM, 0);
  if (listen_sock == kInvalidSocket) {
    return false;
  }

  int reuse = 1;
  ::setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char *>(&reuse), sizeof(reuse));

  sockaddr_in addr{};
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
  addr.sin_port = htons(config_.port);

  if (::bind(listen_sock, reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) != 0 || ::listen(listen_sock, 1) != 0) {
    close_socket(listen_sock);
    return false;
  }

  // Report the actual port (important when config_.port == 0).
  sockaddr_in bound{};
  socklen_t bound_len = sizeof(bound);
  if (::getsockname(listen_sock, reinterpret_cast<sockaddr *>(&bound), &bound_len) == 0) {
    bound_port_.store(ntohs(bound.sin_port), std::memory_order_release);
  } else {
    bound_port_.store(config_.port, std::memory_order_release);
  }

  listen_fd_ = FromSocket(listen_sock);
  running_.store(true, std::memory_order_release);
  thread_ = std::thread([this] { Run(); });
  return true;
}

void AgentControlServer::Stop() {
  if (!running_.exchange(false, std::memory_order_acq_rel)) {
    if (thread_.joinable()) {
      thread_.join();
    }
    return;
  }
  if (listen_fd_ != -1) {
    close_socket(ToSocket(listen_fd_)); // unblocks accept()/select()
    listen_fd_ = -1;
  }
  if (thread_.joinable()) {
    thread_.join();
  }
}

void AgentControlServer::Run() {
  const socket_t listen_sock = ToSocket(listen_fd_);
  while (running_.load(std::memory_order_acquire)) {
    if (!WaitReadable(listen_sock, 200)) {
      continue; // periodic wake to re-check running_
    }
    const socket_t client = ::accept(listen_sock, nullptr, nullptr);
    if (client == kInvalidSocket) {
      continue;
    }
    ServeConnection(FromSocket(client));
    close_socket(client);
  }
}

void AgentControlServer::ServeConnection(std::intptr_t client_fd) {
  const socket_t client = ToSocket(client_fd);
  std::string buffer;
  bool authed = config_.auth_token.empty();

  while (running_.load(std::memory_order_acquire)) {
    if (!WaitReadable(client, 200)) {
      continue;
    }

    char chunk[4096];
    const int n = static_cast<int>(::recv(client, chunk, sizeof(chunk), 0));
    if (n <= 0) {
      return; // client closed or error
    }
    buffer.append(chunk, static_cast<std::size_t>(n));
    if (buffer.size() > kMaxLineBytes) {
      return; // protect against unbounded growth
    }

    // Process every complete newline-terminated line in the buffer.
    std::size_t newline;
    while ((newline = buffer.find('\n')) != std::string::npos) {
      std::string line = buffer.substr(0, newline);
      buffer.erase(0, newline + 1);
      if (!line.empty() && line.back() == '\r') {
        line.pop_back();
      }
      if (line.empty()) {
        continue;
      }

      // Optional auth handshake: first non-empty line must be the token.
      if (!authed) {
        authed = (line == config_.auth_token);
        if (!authed) {
          SendAll(client, agent_SerializeResponse("", AgentResponse::Err("auth failed")) + "\n");
          return;
        }
        continue;
      }

      std::string parse_error;
      std::optional<WireRequest> request = agent_ParseRequest(line, parse_error);
      if (!request.has_value()) {
        SendAll(client, agent_SerializeResponse("", AgentResponse::Err(parse_error)) + "\n");
        continue;
      }

      std::future<AgentResponse> future = queue_.Push(std::move(request->tool), std::move(request->params));

      AgentResponse response;
      if (future.wait_for(std::chrono::milliseconds(config_.request_timeout_ms)) == std::future_status::ready) {
        response = future.get();
      } else {
        response = AgentResponse::Err("timed out waiting for engine main thread");
      }

      if (!SendAll(client, agent_SerializeResponse(request->id, response) + "\n")) {
        return;
      }
    }
  }
}

} // namespace ltjs::agent
