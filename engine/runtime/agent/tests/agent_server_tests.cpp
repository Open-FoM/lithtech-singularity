// Integration tests for AgentControlServer: a real loopback TCP round-trip
// through the wire protocol + command queue. POSIX client; skipped on Windows.

#include <chrono>
#include <string>

#include <gtest/gtest.h>

#include "agent_control.h"
#include "agent_protocol.h"
#include "agent_queue.h"
#include "agent_registry.h"
#include "agent_tcp_server.h"
#include "agent_tool.h"

#ifndef _WIN32
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

using namespace ltjs::agent;

namespace {

AgentTool PingTool() {
  AgentTool t;
  t.name = "ping";
  t.description = "health check";
  t.handler = [](const AgentRequest &) { return AgentResponse::Ok(Json{{"pong", true}}); };
  return t;
}

#ifndef _WIN32
int ConnectLoopback(std::uint16_t port) {
  int fd = ::socket(AF_INET, SOCK_STREAM, 0);
  if (fd < 0)
    return -1;
  sockaddr_in addr{};
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);
  addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
  if (::connect(fd, reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) != 0) {
    ::close(fd);
    return -1;
  }
  return fd;
}

// Pump the queue (as the main thread would) while reading from the socket until
// a full line arrives or the deadline passes.
std::string PumpAndReadLine(AgentCommandQueue &queue, int fd, bool drain) {
  std::string in;
  auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(3);
  while (std::chrono::steady_clock::now() < deadline) {
    if (drain)
      queue.DrainFrame(4);
    fd_set rs;
    FD_ZERO(&rs);
    FD_SET(fd, &rs);
    timeval tv{0, 20000};
    if (::select(fd + 1, &rs, nullptr, nullptr, &tv) > 0 && FD_ISSET(fd, &rs)) {
      char buf[1024];
      int n = static_cast<int>(::recv(fd, buf, sizeof(buf), 0));
      if (n > 0)
        in.append(buf, static_cast<std::size_t>(n));
    }
    const auto nl = in.find('\n');
    if (nl != std::string::npos)
      return in.substr(0, nl);
  }
  return {};
}
#endif

} // namespace

TEST(AgentControlServer, RoundTripPingReturnsResult) {
#ifdef _WIN32
  GTEST_SKIP() << "loopback client uses POSIX sockets";
#else
  AgentRegistry reg;
  reg.Add(PingTool());
  AgentCommandQueue queue(reg);
  AgentControlServer server(queue, AgentControlServer::Config{/*port=*/0});
  ASSERT_TRUE(server.Start());
  ASSERT_NE(server.BoundPort(), 0);

  int fd = ConnectLoopback(server.BoundPort());
  ASSERT_GE(fd, 0);
  const std::string req = std::string(R"({"id":"42","tool":"ping","params":{}})") + "\n";
  ASSERT_EQ(::send(fd, req.data(), req.size(), 0), static_cast<ssize_t>(req.size()));

  const std::string line = PumpAndReadLine(queue, fd, /*drain=*/true);
  ::close(fd);
  server.Stop();

  ASSERT_FALSE(line.empty()) << "no response received";
  Json parsed = Json::parse(line);
  EXPECT_EQ(parsed.at("id").get<std::string>(), "42");
  EXPECT_TRUE(parsed.at("result").at("pong").get<bool>());
#endif
}

TEST(AgentControlServer, MalformedLineReturnsError) {
#ifdef _WIN32
  GTEST_SKIP() << "loopback client uses POSIX sockets";
#else
  AgentRegistry reg;
  AgentCommandQueue queue(reg);
  AgentControlServer server(queue, AgentControlServer::Config{/*port=*/0});
  ASSERT_TRUE(server.Start());

  int fd = ConnectLoopback(server.BoundPort());
  ASSERT_GE(fd, 0);
  const std::string req = "{not valid json\n";
  ASSERT_GT(::send(fd, req.data(), req.size(), 0), 0);

  // No queue drain needed: a parse error is answered directly by the server.
  const std::string line = PumpAndReadLine(queue, fd, /*drain=*/false);
  ::close(fd);
  server.Stop();

  ASSERT_FALSE(line.empty()) << "no response received";
  Json parsed = Json::parse(line);
  EXPECT_TRUE(parsed.contains("error"));
#endif
}

TEST(AgentControl, InitRegistersCoreToolsAndStartsServer) {
  agent_DefaultRegistry().Clear();

  AgentControlConfig cfg;
  cfg.port = 0; // ephemeral; avoids clashing with a real instance
  agent_control_Init(cfg);

  EXPECT_TRUE(agent_control_IsRunning());
  ASSERT_NE(agent_DefaultRegistry().Find("ping"), nullptr);
  ASSERT_NE(agent_DefaultRegistry().Find("__list_tools"), nullptr);

  const AgentTool *ping = agent_DefaultRegistry().Find("ping");
  Json params = Json::object();
  AgentResponse r = ping->handler(AgentRequest{"ping", params});
  EXPECT_TRUE(r.ok);
  EXPECT_TRUE(r.result.at("pong").get<bool>());

  agent_control_Shutdown();
  EXPECT_FALSE(agent_control_IsRunning());
}
