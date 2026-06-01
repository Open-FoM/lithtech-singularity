#include "agent_control.h"

#include <cstdio>
#include <memory>
#include <utility>

#include "agent_protocol.h"
#include "agent_queue.h"
#include "agent_registry.h"
#include "agent_tcp_server.h"
#include "agent_tool.h"

namespace ltjs::agent {

namespace {

// The command queue is bound to the process-wide default registry, which is
// where both the built-ins below and game-code tools (agent_AddTool) live.
AgentCommandQueue &ControlQueue() {
  static AgentCommandQueue queue(agent_DefaultRegistry());
  return queue;
}

std::unique_ptr<AgentControlServer> &ServerSlot() {
  static std::unique_ptr<AgentControlServer> server;
  return server;
}

void RegisterCoreTools() {
  AgentTool ping;
  ping.name = "ping";
  ping.description = "Liveness check; returns {pong:true}.";
  ping.handler = [](const AgentRequest &) { return AgentResponse::Ok(Json{{"pong", true}}); };
  agent_AddTool(std::move(ping)); // returns false if already present — fine.

  AgentTool list;
  list.name = "__list_tools";
  list.description = "List every registered tool and its parameter schema.";
  list.handler = [](const AgentRequest &) { return AgentResponse::Ok(agent_SerializeToolList(agent_GetTools())); };
  agent_AddTool(std::move(list));
}

// Ensures Shutdown() runs even if the engine never calls it explicitly.
struct ShutdownGuard {
  ~ShutdownGuard() { agent_control_Shutdown(); }
};
ShutdownGuard g_shutdown_guard;

} // namespace

void agent_control_Init(const AgentControlConfig &config) {
  std::unique_ptr<AgentControlServer> &server = ServerSlot();
  if (server && server->IsRunning()) {
    return;
  }

  RegisterCoreTools();

  AgentControlServer::Config server_config;
  server_config.port = config.port;
  server_config.auth_token = config.auth_token;
  server_config.request_timeout_ms = config.request_timeout_ms;

  server = std::make_unique<AgentControlServer>(ControlQueue(), server_config);
  if (server->Start()) {
    std::fprintf(stderr, "[agent] control surface listening on 127.0.0.1:%u\n",
                 static_cast<unsigned>(server->BoundPort()));
  } else {
    std::fprintf(stderr, "[agent] failed to bind control port %u; agent surface disabled\n",
                 static_cast<unsigned>(config.port));
    server.reset();
  }
}

void agent_control_PumpFrame(int max_per_frame) {
  std::unique_ptr<AgentControlServer> &server = ServerSlot();
  if (server && server->IsRunning()) {
    ControlQueue().DrainFrame(max_per_frame);
  }
}

void agent_control_Shutdown() {
  std::unique_ptr<AgentControlServer> &server = ServerSlot();
  if (server) {
    server->Stop();
    server.reset();
  }
}

bool agent_control_IsRunning() {
  std::unique_ptr<AgentControlServer> &server = ServerSlot();
  return server && server->IsRunning();
}

} // namespace ltjs::agent
