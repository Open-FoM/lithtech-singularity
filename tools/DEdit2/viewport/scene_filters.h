#pragma once

#include <string>

struct NodeProperties;
struct ViewportPanelState;

struct NodeCategoryFlags
{
  bool world = false;
  bool world_model = false;
  bool model = false;
  bool sprite = false;
  bool polygrid = false;
  bool particles = false;
  bool volume = false;
  bool line_system = false;
  bool canvas = false;
  bool sky = false;
};

NodeCategoryFlags ClassifyNodeCategories(const NodeProperties& props);
bool NodePickableByRender(const ViewportPanelState& viewport_state, const NodeProperties& props);
bool IsLightNode(const NodeProperties& props);
bool IsDirectionalLightNode(const NodeProperties& props);
