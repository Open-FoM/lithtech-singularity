#include "bdefs.h"

#include <cstdint>
#include <cstdlib>
#include <string>
#include <vector>

#include "agent_engine_tools.h"

#include "concommand.h"
#include "console.h"
#include "consolecommands.h"
#include "icommandlineargs.h"

#include "agent_registry.h"
#include "agent_tool.h"

// Command-line access via the engine module holder (same pattern as client.cpp).
// The generated holder symbols have internal linkage, so a second holder for the
// same interface in this TU does not clash with the one in client.cpp.
static ICommandLineArgs *g_agent_command_line_args = nullptr;
define_holder(ICommandLineArgs, g_agent_command_line_args);

namespace ltjs::agent {

namespace {

// Console output capture. The engine routes console text through the error-log
// callback; we install one for the duration of a single command. Single-threaded
// (runs on the main thread inside DrainFrame), but guarded against re-entrancy
// in case a command itself invokes console output handling.
thread_local std::vector<std::string> *tls_capture = nullptr;

void CaptureConsoleLine(const char *message) {
  if (tls_capture != nullptr && message != nullptr) {
    tls_capture->emplace_back(message);
  }
}

AgentResponse ConsoleExec(const AgentRequest &request) {
  if (!request.params.contains("command") || !request.params.at("command").is_string()) {
    return AgentResponse::Err("console_exec requires a string 'command'");
  }
  const std::string command = request.params.at("command").get<std::string>();

  if (tls_capture != nullptr) {
    return AgentResponse::Err("console_exec is not re-entrant");
  }

  std::vector<std::string> captured;
  tls_capture = &captured;
  con_SetErrorLog(&CaptureConsoleLine);
  c_CommandHandler(command.c_str());
  con_SetErrorLog(nullptr);
  tls_capture = nullptr;

  Json result;
  result["command"] = command;
  result["output"] = captured; // best-effort: not all output flows through the error log
  return AgentResponse::Ok(std::move(result));
}

AgentResponse GetCvar(const AgentRequest &request) {
  if (!request.params.contains("name") || !request.params.at("name").is_string()) {
    return AgentResponse::Err("get_cvar requires a string 'name'");
  }
  const std::string name = request.params.at("name").get<std::string>();

  LTCommandVar *var = cc_FindConsoleVar(&g_ClientConsoleState, name.c_str());
  if (var == nullptr) {
    return AgentResponse::Err("no such console variable: " + name);
  }

  Json result;
  result["name"] = name;
  result["float"] = var->floatVal;
  result["value"] = var->pStringVal != nullptr ? std::string(var->pStringVal) : std::string();
  return AgentResponse::Ok(std::move(result));
}

AgentResponse SetCvar(const AgentRequest &request) {
  if (!request.params.contains("name") || !request.params.at("name").is_string()) {
    return AgentResponse::Err("set_cvar requires a string 'name'");
  }
  const std::string name = request.params.at("name").get<std::string>();

  std::string value;
  const Json provided = request.params.contains("value") ? request.params.at("value") : Json();
  if (provided.is_string()) {
    value = provided.get<std::string>();
  } else if (provided.is_number()) {
    value = provided.dump();
  } else if (provided.is_boolean()) {
    value = provided.get<bool>() ? "1" : "0";
  } else {
    return AgentResponse::Err("set_cvar requires 'value' as a string, number or boolean");
  }

  cc_SetConsoleVariable(&g_ClientConsoleState, name.c_str(), value.c_str());

  Json result;
  result["name"] = name;
  result["value"] = value;
  result["ok"] = true;
  return AgentResponse::Ok(std::move(result));
}

AgentTool MakeTool(std::string name, std::string description, std::vector<AgentParamDesc> params, AgentToolFn handler) {
  AgentTool tool;
  tool.name = std::move(name);
  tool.description = std::move(description);
  tool.params = std::move(params);
  tool.handler = std::move(handler);
  return tool;
}

// Returns a non-empty argument value for the given "--name" flag (matched via
// FindArgDash, which prepends one dash to the supplied name), or nullptr.
const char *FindDoubleDashArg(const char *name_without_leading_dashes) {
  if (g_agent_command_line_args == nullptr) {
    return nullptr;
  }
  const char *value = g_agent_command_line_args->FindArgDash(name_without_leading_dashes);
  return (value != nullptr && value[0] != '\0') ? value : nullptr;
}

} // namespace

void agent_RegisterEngineTools() {
  agent_AddTool(MakeTool("console_exec", "Run a client console command. Output capture is best-effort.",
                         {{"command", "string", "The console command line to execute", true}}, &ConsoleExec));

  agent_AddTool(MakeTool("get_cvar", "Read a console variable's string and float value.",
                         {{"name", "string", "Console variable name", true}}, &GetCvar));

  agent_AddTool(MakeTool("set_cvar", "Set a console variable.",
                         {{"name", "string", "Console variable name", true},
                          {"value", "string", "New value (string, number or boolean)", true}},
                         &SetCvar));
}

bool agent_GetMcpServerConfig(AgentControlConfig &out) {
  // Command line: "--mcp-port <n>". FindArgDash("-mcp-port") forms "--mcp-port".
  const char *port_str = FindDoubleDashArg("-mcp-port");
  const char *token_str = FindDoubleDashArg("-mcp-token");

  // Env fallbacks.
  if (port_str == nullptr) {
    const char *env_port = std::getenv("LTJS_AGENT_PORT");
    if (env_port != nullptr && env_port[0] != '\0') {
      port_str = env_port;
    }
  }
  if (token_str == nullptr) {
    const char *env_token = std::getenv("LTJS_AGENT_TOKEN");
    if (env_token != nullptr && env_token[0] != '\0') {
      token_str = env_token;
    }
  }

  if (port_str == nullptr) {
    return false; // operator did not request the control surface
  }

  const int port = std::atoi(port_str);
  if (port <= 0 || port > 65535) {
    return false;
  }

  out.port = static_cast<std::uint16_t>(port);
  if (token_str != nullptr) {
    out.auth_token = token_str;
  }
  return true;
}

} // namespace ltjs::agent
