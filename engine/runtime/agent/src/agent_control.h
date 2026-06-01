// ------------------------------------------------------------------ //
//  FILE    : agent_control.h
//  PURPOSE : Lifecycle facade for the agent control surface.
//
//  The agent module is always compiled into the engine; whether it *runs* is a
//  runtime decision. The engine calls agent_control_Init(config) only when the
//  operator requested it (e.g. via --mcp-port); otherwise the server never
//  starts and PumpFrame() is a cheap no-op.
//
//      agent_control_Init(config)  once, after tools are registered;
//      agent_control_PumpFrame()   every frame on the main thread;
//      agent_control_Shutdown()    on teardown (also runs automatically at exit).
//
//  Engine-free: built-in tools registered here (ping, __list_tools) touch only
//  the registry, so this whole translation unit stays unit-testable.
// ------------------------------------------------------------------ //

#pragma once

#include <cstdint>
#include <string>

namespace ltjs::agent {

struct AgentControlConfig {
  std::uint16_t port = 0; ///< TCP port to listen on (0 lets the OS pick).
  std::string auth_token; ///< Empty disables auth; else the first client line must match.
  int request_timeout_ms = 2000;
};

/// Register core built-in tools (ping, __list_tools) and start the TCP server on
/// the configured port. Idempotent: a second call while running is a no-op.
void agent_control_Init(const AgentControlConfig &config);

/// Drain queued tool calls on the calling (main) thread. No-op if not started.
void agent_control_PumpFrame(int max_per_frame = 4);

/// Stop the server (also invoked automatically at process exit). Idempotent.
void agent_control_Shutdown();

/// Whether the control server is currently running.
bool agent_control_IsRunning();

} // namespace ltjs::agent
