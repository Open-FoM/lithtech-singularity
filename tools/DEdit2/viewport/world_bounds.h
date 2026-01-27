#pragma once

#include <vector>

struct NodeProperties;

bool TryGetWorldBounds(const std::vector<NodeProperties>& props, float out_min[3], float out_max[3]);
