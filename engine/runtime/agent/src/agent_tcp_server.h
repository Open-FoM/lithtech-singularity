// ------------------------------------------------------------------ //
//  FILE    : agent_tcp_server.h
//  PURPOSE : Localhost TCP front-end for the agent control surface.
//
//  Speaks the newline-delimited JSON wire protocol (see agent_protocol.h) to an
//  external MCP sidecar. Runs its accept/serve loop on a dedicated transport
//  thread: each request is parsed, pushed onto the AgentCommandQueue, and
//  awaited (with a timeout) until the engine main thread drains it. The server
//  therefore never touches engine state itself — only the queue.
//
//  Bound to loopback only. A single connection is served at a time, which is
//  all a dev utility needs; the loop re-accepts after a client disconnects.
// ------------------------------------------------------------------ //

#pragma once

#include <atomic>
#include <cstdint>
#include <string>
#include <thread>

namespace ltjs::agent {

class AgentCommandQueue;

class AgentControlServer {
public:
  struct Config {
    std::uint16_t port = 27182;    ///< 0 lets the OS pick (BoundPort() reports it).
    std::string auth_token;        ///< Empty disables auth; else the first line must be the token.
    int request_timeout_ms = 2000; ///< How long a request waits for the main thread to drain it.
  };

  AgentControlServer(AgentCommandQueue &queue, Config config);
  ~AgentControlServer();

  AgentControlServer(const AgentControlServer &) = delete;
  AgentControlServer &operator=(const AgentControlServer &) = delete;

  /// Bind + listen + spawn the transport thread. Returns false if the socket
  /// could not be bound (e.g. port in use); the server is then inert.
  bool Start();

  /// Signal the transport thread to stop and join it. Idempotent.
  void Stop();

  bool IsRunning() const { return running_.load(std::memory_order_acquire); }

  /// The actually-bound port (meaningful after a successful Start()).
  std::uint16_t BoundPort() const { return bound_port_.load(std::memory_order_acquire); }

private:
  void Run();
  void ServeConnection(std::intptr_t client_fd);

  AgentCommandQueue &queue_;
  Config config_;
  std::atomic<bool> running_{false};
  std::atomic<std::uint16_t> bound_port_{0};
  std::intptr_t listen_fd_ = -1; ///< Holds a SOCKET (Windows) or int fd (POSIX).
  std::thread thread_;
};

} // namespace ltjs::agent
