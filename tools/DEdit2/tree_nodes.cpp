#include "editor_state.h"

const char* GetNodeName(const std::vector<TreeNode>& nodes, int node_id)
{
	if (node_id < 0 || node_id >= static_cast<int>(nodes.size()))
	{
		return "None";
	}
	if (nodes[node_id].deleted)
	{
		return "None";
	}
	return nodes[node_id].name.c_str();
}

int AddChildNode(
	std::vector<TreeNode>& nodes,
	std::vector<NodeProperties>& props,
	int parent_id,
	const char* name,
	bool is_folder,
	const char* type_name)
{
	TreeNode node;
	node.name = name;
	node.is_folder = is_folder;
	node.deleted = false;
	const int new_id = static_cast<int>(nodes.size());
	nodes.push_back(std::move(node));
	props.push_back(MakeProps(type_name));
	if (!is_folder && type_name)
	{
		props.back().resource = name;
	}
	if (parent_id >= 0 && parent_id < static_cast<int>(nodes.size()))
	{
		nodes[parent_id].children.push_back(new_id);
	}
	return new_id;
}
