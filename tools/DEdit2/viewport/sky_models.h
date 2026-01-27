#pragma once

#include <string>
#include <vector>

struct DiligentContext;
struct NodeProperties;
struct TreeNode;

void BuildSkyWorldModels(
  DiligentContext& ctx,
  const std::vector<TreeNode>& nodes,
  const std::vector<NodeProperties>& props,
  const std::string& selection);
