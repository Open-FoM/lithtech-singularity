#include "agent_protocol.h"

#include <exception>
#include <string>
#include <utility>

namespace ltjs::agent {

std::optional<WireRequest> agent_ParseRequest(std::string_view line, std::string &error_out) {
  Json parsed;
  try {
    parsed = Json::parse(line);
  } catch (const std::exception &e) {
    error_out = std::string("invalid JSON: ") + e.what();
    return std::nullopt;
  }

  if (!parsed.is_object()) {
    error_out = "request must be a JSON object";
    return std::nullopt;
  }

  const auto tool_it = parsed.find("tool");
  if (tool_it == parsed.end() || !tool_it->is_string() || tool_it->get<std::string>().empty()) {
    error_out = "request is missing a non-empty \"tool\" field";
    return std::nullopt;
  }

  WireRequest request;
  request.tool = tool_it->get<std::string>();

  if (const auto id_it = parsed.find("id"); id_it != parsed.end() && id_it->is_string()) {
    request.id = id_it->get<std::string>();
  }

  if (const auto params_it = parsed.find("params"); params_it != parsed.end() && params_it->is_object()) {
    request.params = *params_it;
  } else {
    request.params = Json::object();
  }

  return request;
}

std::string agent_SerializeResponse(std::string_view id, const AgentResponse &resp) {
  Json out;
  out["id"] = std::string(id);
  if (resp.ok) {
    out["result"] = resp.result;
  } else {
    out["error"] = resp.error;
  }
  return out.dump();
}

Json agent_SerializeToolList(std::span<const AgentTool> tools) {
  Json array = Json::array();
  for (const AgentTool &tool : tools) {
    Json params = Json::array();
    for (const AgentParamDesc &param : tool.params) {
      params.push_back(Json{{"name", param.name},
                            {"type", param.type},
                            {"description", param.description},
                            {"required", param.required}});
    }
    array.push_back(Json{{"name", tool.name}, {"description", tool.description}, {"parameters", std::move(params)}});
  }
  return array;
}

} // namespace ltjs::agent
