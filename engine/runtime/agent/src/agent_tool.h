// ------------------------------------------------------------------ //
//  FILE    : agent_tool.h
//  PURPOSE : Core value types for the agent control surface.
//
//  This header is the transport-agnostic vocabulary shared by every
//  front-end (the MCP TCP server today; a Lua binder later). A "tool" is a
//  named, parameterised operation whose handler runs ON THE ENGINE MAIN
//  THREAD. Tools are registered into an AgentRegistry (see agent_registry.h)
//  and invoked through an AgentCommandQueue (see agent_queue.h).
//
//  Intentionally free of any engine headers so the core stays unit-testable
//  in isolation; the only dependencies are the STL and nlohmann::json.
// ------------------------------------------------------------------ //

#pragma once

#include <functional>
#include <string>
#include <utility>
#include <vector>

#include <nlohmann/json.hpp>

namespace ltjs::agent {

using Json = nlohmann::json;

/// Describes a single tool parameter, used to generate the MCP input schema.
struct AgentParamDesc {
  std::string name;
  std::string type; ///< JSON-schema scalar: "string" | "number" | "integer" | "boolean"
  std::string description;
  bool required = true;
};

/// The result returned by a tool handler.
///
/// Use the Ok()/Err() factories rather than aggregate-initialising directly so
/// the invariant "error is non-empty iff !ok" stays in one place.
struct AgentResponse {
  bool ok = true;
  Json result;       ///< Arbitrary payload when ok == true.
  std::string error; ///< Human-readable message when ok == false.

  static AgentResponse Ok(Json result = Json::object()) {
    return AgentResponse{true, std::move(result), std::string{}};
  }
  static AgentResponse Err(std::string message) { return AgentResponse{false, Json{}, std::move(message)}; }
};

/// The arguments handed to a tool handler. `params` references storage owned by
/// the caller (the command queue) for the duration of the call only.
struct AgentRequest {
  std::string_view tool_name;
  const Json &params;
};

/// A tool handler. Runs on the engine main thread; may freely touch engine
/// state. Must not block (it stalls the frame).
using AgentToolFn = std::function<AgentResponse(const AgentRequest &)>;

/// A registered tool: identity, human/LLM-facing docs, schema, and behaviour.
struct AgentTool {
  std::string name;
  std::string description;
  std::vector<AgentParamDesc> params;
  AgentToolFn handler;
};

} // namespace ltjs::agent
