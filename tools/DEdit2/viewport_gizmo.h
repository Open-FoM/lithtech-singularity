#pragma once

#include "editor_state.h"
#include "ui_viewport.h"
#include "undo_stack.h"

#include "DiligentCore/Common/interface/BasicMath.hpp"
#include "imgui.h"

#include <vector>

struct ScenePanelState;

struct GizmoHandle
{
	ViewportPanelState::GizmoAxis axis = ViewportPanelState::GizmoAxis::None;
	int axis_index = -1;
	Diligent::float3 dir = Diligent::float3(0.0f, 0.0f, 0.0f);
	Diligent::float3 ring_u = Diligent::float3(0.0f, 0.0f, 0.0f);
	Diligent::float3 ring_v = Diligent::float3(0.0f, 0.0f, 0.0f);
	ImVec2 start = ImVec2(0.0f, 0.0f);
	ImVec2 end = ImVec2(0.0f, 0.0f);
	float screen_len = 0.0f;
	float world_len = 0.0f;
};

struct GizmoDrawState
{
	bool origin_visible = false;
	ImVec2 origin_screen = ImVec2(0.0f, 0.0f);
	ImVec2 viewport_size = ImVec2(0.0f, 0.0f);
	Diligent::float4x4 view_proj = Diligent::float4x4::Identity();
	Diligent::float3 origin = Diligent::float3(0.0f, 0.0f, 0.0f);
	GizmoHandle handles[3];

	/// Multi-selection tracking.
	bool is_multi_select = false;
	std::vector<int> affected_node_ids;
	std::vector<TransformState> start_transforms;
	std::vector<std::vector<float>> start_vertices;  ///< Brush vertices at drag start
};

bool ProjectWorldToScreen(
	const Diligent::float4x4& view_proj,
	const float position[3],
	const ImVec2& viewport_size,
	ImVec2& out_screen);

bool BuildGizmoDrawState(
	const ViewportPanelState& panel,
	const NodeProperties& props,
	const Diligent::float3& cam_pos,
	const Diligent::float4x4& view_proj,
	const ImVec2& viewport_size,
	GizmoDrawState& out_state);

void DrawGizmo(
	const ViewportPanelState& panel,
	const GizmoDrawState& state,
	const ImVec2& viewport_pos,
	ImDrawList* draw_list);

bool UpdateGizmoInteraction(
	ViewportPanelState& panel,
	const GizmoDrawState& state,
	NodeProperties& props,
	const ImVec2& mouse_local,
	bool mouse_down,
	bool mouse_clicked);

/// Build gizmo state for multi-selection (uses selection center as origin).
/// @param panel Viewport panel state.
/// @param scene_panel Scene panel with selection info.
/// @param nodes Scene tree nodes.
/// @param props Scene node properties.
/// @param cam_pos Camera position.
/// @param view_proj View-projection matrix.
/// @param viewport_size Viewport dimensions.
/// @param out_state Output gizmo state.
/// @return true if gizmo should be rendered.
bool BuildGizmoDrawStateMultiSelect(
	const ViewportPanelState& panel,
	const ScenePanelState& scene_panel,
	const std::vector<TreeNode>& nodes,
	const std::vector<NodeProperties>& props,
	const Diligent::float3& cam_pos,
	const Diligent::float4x4& view_proj,
	const ImVec2& viewport_size,
	GizmoDrawState& out_state);

/// Update gizmo interaction for multi-selection.
/// Transforms all selected nodes and pushes undo action on completion.
/// @param panel Viewport panel state.
/// @param state Gizmo draw state (updated with start transforms on drag begin).
/// @param scene_panel Scene panel with selection info.
/// @param nodes Scene tree nodes.
/// @param props Scene node properties (modified during drag).
/// @param mouse_local Mouse position relative to viewport.
/// @param mouse_down Left mouse button held.
/// @param mouse_clicked Left mouse button just clicked.
/// @param undo_stack Undo stack for pushing transform changes.
/// @return true if gizmo consumed the click.
bool UpdateGizmoInteractionMultiSelect(
	ViewportPanelState& panel,
	GizmoDrawState& state,
	const ScenePanelState& scene_panel,
	const std::vector<TreeNode>& nodes,
	std::vector<NodeProperties>& props,
	const ImVec2& mouse_local,
	bool mouse_down,
	bool mouse_clicked,
	UndoStack* undo_stack);
