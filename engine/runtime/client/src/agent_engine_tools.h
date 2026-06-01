// ------------------------------------------------------------------ //
//  FILE    : agent_engine_tools.h
//  PURPOSE : Engine-coupled glue for the agent control surface.
//
//  The agent module is always built. These helpers live in the client module
//  (rather than the engine-free agent library) because they reach into client
//  globals: the console tools touch g_ClientConsoleState, and the config
//  resolver reads the engine command line via ICommandLineArgs.
// ------------------------------------------------------------------ //

#pragma once

#include "agent_control.h"

namespace ltjs::agent {

/// Register console_exec, get_cvar and set_cvar into the default registry.
void agent_RegisterEngineTools();

/// Decide whether the operator asked for the control server and on what port.
/// Reads the `--mcp-port <n>` command-line flag first, then the LTJS_AGENT_PORT
/// env var; `--mcp-token`/LTJS_AGENT_TOKEN supply an optional auth token.
/// Returns true (and fills `out`) only when a valid port was requested.
bool agent_GetMcpServerConfig(AgentControlConfig &out);

} // namespace ltjs::agent
