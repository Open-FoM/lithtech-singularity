#include "app/project_utils.h"

#include "editor_state.h"

int DefaultProjectSelection(const std::vector<TreeNode>& nodes)
{
  if (nodes.empty())
  {
    return -1;
  }
  if (nodes[0].children.empty())
  {
    return -1;
  }
  return nodes[0].children.front();
}
