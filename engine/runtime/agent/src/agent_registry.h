// ------------------------------------------------------------------ //
//  FILE    : agent_registry.h
//  PURPOSE : The set of tools an agent (or script) may invoke.
//
//  The registry is the shared facade: MCP and, later, Lua are two front-ends
//  over the same tool set. It is intended to be populated once at startup
//  (engine built-ins first, then game-code tools via agent_AddTool) and read
//  concurrently afterwards, so it carries no internal locking — callers must
//  finish registration before the transport thread starts.
// ------------------------------------------------------------------ //

#pragma once

#include <span>
#include <string_view>
#include <vector>

#include "agent_tool.h"

namespace ltjs::agent {

/// An ordered, name-unique collection of tools.
class AgentRegistry {
public:
  /// Register a tool. Returns false (and leaves the registry unchanged) if a
  /// tool with the same name already exists.
  bool Add(AgentTool tool);

  /// Find a tool by exact name, or nullptr. The pointer is stable until the
  /// registry is mutated.
  const AgentTool *Find(std::string_view name) const;

  /// All registered tools, in registration order.
  std::span<const AgentTool> Tools() const;

  std::size_t Size() const { return tools_.size(); }

  /// Remove all tools. Primarily for tests.
  void Clear();

private:
  std::vector<AgentTool> tools_;
};

/// Process-wide registry backing the game-code-facing convenience API.
AgentRegistry &agent_DefaultRegistry();

/// Game-code-facing registration, mirroring cc_AddCommand. Operates on the
/// default registry. Returns false on duplicate name.
bool agent_AddTool(AgentTool tool);

/// All tools in the default registry.
std::span<const AgentTool> agent_GetTools();

} // namespace ltjs::agent
