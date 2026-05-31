#pragma once

#include "editor_state.h"
#include "geometry/edit_mode.h"
#include "geometry/subobject_selection.h"
#include "ui_face_properties.h"

/// Result from drawing the properties panel.
struct PropertiesPanelResult
{
  bool browse_texture = false;     ///< User wants to open texture browser
  FacePropertiesAction face_action; ///< Action from face properties section
};

void DrawPropertiesPanel(
	SelectionTarget active_target,
	std::vector<TreeNode>& project_nodes,
	std::vector<NodeProperties>& project_props,
	int project_selected_id,
	std::vector<TreeNode>& scene_nodes,
	std::vector<NodeProperties>& scene_props,
	int scene_selected_id,
	const std::string& project_root,
	EditMode current_edit_mode,
	SubObjectSelection& face_selection,
	FacePropertiesPanel& face_panel,
	const PickedTextureState& picked_texture,
	PropertiesPanelResult& result);
