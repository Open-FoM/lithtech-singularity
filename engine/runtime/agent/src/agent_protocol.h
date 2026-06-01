// ------------------------------------------------------------------ //
//  FILE    : agent_protocol.h
//  PURPOSE : The newline-delimited JSON wire protocol the engine speaks to the
//            external MCP sidecar.
//
//  Each request is one JSON object on a single line:
//      {"id": "<corr-id>", "tool": "<name>", "params": { ... }}
//  Each response is one JSON object on a single line, correlated by id:
//      {"id": "<corr-id>", "result": <any-json>}          // success
//      {"id": "<corr-id>", "error":  "<message>"}          // failure
//
//  Parsing/serialisation is pure and engine-independent so it can be unit
//  tested without a running engine.
// ------------------------------------------------------------------ //

#pragma once

#include <optional>
#include <span>
#include <string>
#include <string_view>

#include "agent_tool.h"

namespace ltjs::agent {

/// A decoded request line.
struct WireRequest {
  std::string id;
  std::string tool;
  Json params = Json::object(); ///< Always an object; absent/null becomes {}.
};

/// Parse one request line. On malformed input returns nullopt and writes a
/// reason into `error_out`. A missing "params" is allowed (defaults to {}); a
/// missing/empty "tool" or non-object top level is an error.
std::optional<WireRequest> agent_ParseRequest(std::string_view line, std::string &error_out);

/// Serialise a response correlated to `id`. Produces a {"id","result"} object
/// when resp.ok, otherwise {"id","error"}. No trailing newline.
std::string agent_SerializeResponse(std::string_view id, const AgentResponse &resp);

/// Build the JSON array describing `tools` for the sidecar's tools/list. Each
/// entry is {"name","description","parameters":[{name,type,description,required}]}.
Json agent_SerializeToolList(std::span<const AgentTool> tools);

} // namespace ltjs::agent
