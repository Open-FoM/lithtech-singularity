#include "viewport/world_bounds.h"

#include "editor_state.h"

#include <sstream>

namespace
{
bool ParseFloat3(const std::string& text, float out[3])
{
  std::istringstream stream(text);
  return static_cast<bool>(stream >> out[0] >> out[1] >> out[2]);
}

bool ExtractWorldBoundsFromProps(const NodeProperties& props, float out_min[3], float out_max[3])
{
  bool has_min = false;
  bool has_max = false;
  for (const auto& entry : props.raw_properties)
  {
    if (entry.first == "world_bounds_min")
    {
      has_min = ParseFloat3(entry.second, out_min);
    }
    else if (entry.first == "world_bounds_max")
    {
      has_max = ParseFloat3(entry.second, out_max);
    }
    if (has_min && has_max)
    {
      return true;
    }
  }
  return false;
}
}

bool TryGetWorldBounds(const std::vector<NodeProperties>& props, float out_min[3], float out_max[3])
{
  if (props.empty())
  {
    return false;
  }
  if (ExtractWorldBoundsFromProps(props.front(), out_min, out_max))
  {
    return true;
  }
  for (size_t i = 1; i < props.size(); ++i)
  {
    if (ExtractWorldBoundsFromProps(props[i], out_min, out_max))
    {
      return true;
    }
  }
  return false;
}
