#include "agent_registry.h"

#include <utility>

namespace ltjs::agent {

bool AgentRegistry::Add(AgentTool tool) {
  if (Find(tool.name) != nullptr) {
    return false;
  }
  tools_.push_back(std::move(tool));
  return true;
}

const AgentTool *AgentRegistry::Find(std::string_view name) const {
  // Linear scan: the tool set is small (dozens at most) and registration
  // happens once at startup, so a hash index would be premature.
  for (const AgentTool &tool : tools_) {
    if (tool.name == name) {
      return &tool;
    }
  }
  return nullptr;
}

std::span<const AgentTool> AgentRegistry::Tools() const { return tools_; }

void AgentRegistry::Clear() { tools_.clear(); }

AgentRegistry &agent_DefaultRegistry() {
  static AgentRegistry registry;
  return registry;
}

bool agent_AddTool(AgentTool tool) { return agent_DefaultRegistry().Add(std::move(tool)); }

std::span<const AgentTool> agent_GetTools() { return agent_DefaultRegistry().Tools(); }

} // namespace ltjs::agent
