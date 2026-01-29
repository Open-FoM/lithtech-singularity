#pragma once

#include "imgui.h"

#include <string>

struct ViewportPanelState
{
	bool initialized = false;
	bool show_grid = true;
	bool show_axes = true;

	/// Viewport projection mode.
	enum class ViewMode
	{
		Perspective,  ///< Standard perspective view with orbit/fly controls
		Top,          ///< Orthographic looking down -Y axis
		Bottom,       ///< Orthographic looking up +Y axis
		Front,        ///< Orthographic looking down -Z axis
		Back,         ///< Orthographic looking down +Z axis
		Left,         ///< Orthographic looking down +X axis
		Right         ///< Orthographic looking down -X axis
	};
	ViewMode view_mode = ViewMode::Perspective;

	// Perspective mode state
	float orbit_yaw = 0.0f;
	float orbit_pitch = 0.0f;
	float orbit_distance = 800.0f;
	float target[3] = {0.0f, 0.0f, 0.0f};
	bool fly_mode = false;

	// Orthographic mode state
	float ortho_zoom = 1.0f;            ///< Zoom level (smaller = zoomed in)
	float ortho_center[2] = {0.0f, 0.0f}; ///< 2D pan center in world coordinates
	float ortho_depth = 0.0f;           ///< Depth coordinate preserved when switching views
	ViewMode last_ortho_view = ViewMode::Top; ///< Last used ortho view for toggle
	float fly_pos[3] = {0.0f, 0.0f, 0.0f};
	float fly_speed = 800.0f;
	float grid_spacing = 64.0f;
	float grid_origin[3] = {0.0f, 0.0f, 0.0f};
	enum class GizmoMode
	{
		Translate,
		Rotate,
		Scale
	};
	enum class GizmoAxis
	{
		None,
		X,
		Y,
		Z
	};
	enum class GizmoSpace
	{
		World,
		Local
	};
	GizmoMode gizmo_mode = GizmoMode::Translate;
	GizmoAxis gizmo_axis = GizmoAxis::None;
	GizmoSpace gizmo_space = GizmoSpace::World;
	GizmoSpace rotation_space = GizmoSpace::World;
	bool gizmo_dragging = false;
	ImVec2 gizmo_drag_start_mouse = ImVec2(0.0f, 0.0f);
	float gizmo_drag_start_angle = 0.0f;
	float gizmo_start_pos[3] = {0.0f, 0.0f, 0.0f};
	float gizmo_start_rot[3] = {0.0f, 0.0f, 0.0f};
	float gizmo_start_scale[3] = {1.0f, 1.0f, 1.0f};
	float gizmo_size = 1.0f;
	float gizmo_axis_scale[3] = {1.0f, 1.0f, 1.0f};
	bool snap_translate = false;
	bool snap_rotate = false;
	bool snap_scale = false;
	bool snap_translate_axis[3] = {true, true, true};
	bool snap_rotate_axis[3] = {true, true, true};
	bool snap_scale_axis[3] = {true, true, true};
	float snap_translate_step = 1.0f;
	float snap_rotate_step = 15.0f;
	float snap_scale_step = 0.1f;
	bool render_draw_world = true;
	bool render_draw_world_models = true;
	bool render_draw_models = true;
	bool render_draw_sprites = true;
	bool render_draw_polygrids = true;
	bool render_draw_particles = true;
	bool render_draw_volume_effects = true;
	bool render_draw_line_systems = true;
	bool render_draw_canvases = true;
	bool render_draw_sky = true;
	std::string sky_world_model;
	bool render_wireframe_world = false;
	bool render_wireframe_models = false;
	int render_world_shading_mode = 0;
	bool render_wireframe_overlay = false;
	enum class WorldLightmapMode
	{
		Dynamic = 0,
		Baked = 1,
		LightmapOnly = 2
	};
	int render_lightmap_mode = static_cast<int>(WorldLightmapMode::Dynamic);
	bool render_lightmaps_available = false;
	float render_lightmap_intensity = 1.0f;
	bool render_fullbright = false;
	bool render_draw_sorted = true;
	bool render_draw_solid_models = true;
	bool render_draw_translucent_models = true;
	bool render_texture_models = true;
	bool render_light_models = true;
	bool render_model_apply_ambient = true;
	bool render_model_apply_sun = true;
	float render_model_saturation = 1.0f;
	bool render_dynamic_lights = true;
	bool render_dynamic_lights_world = true;
	bool render_polygrid_env_map = true;
	bool render_polygrid_bump_map = true;
	int render_model_lod_offset = 0;
	bool render_model_debug_boxes = false;
	bool render_model_debug_touching_lights = false;
	bool render_model_debug_skeleton = false;
	bool render_model_debug_obbs = false;
	bool render_model_debug_vertex_normals = false;
	bool render_world_debug_nodes = false;
	bool render_world_debug_leaves = false;
	bool render_world_debug_portals = false;
	bool render_screen_glow_enable = false;
	bool render_screen_glow_show_texture = false;
	bool render_screen_glow_show_filter = false;
	float render_screen_glow_show_texture_scale = 0.5f;
	float render_screen_glow_show_filter_scale = 0.25f;
	float render_screen_glow_uv_scale = 0.75f;
	int render_screen_glow_texture_size = 256;
	int render_screen_glow_filter_size = 28;
	bool render_ssao_enable = false;
	int render_ssao_backend = 1;
	float render_ssao_intensity = 1.0f;
	float render_ssao_fx_effect_radius = 60.0f;
	float render_ssao_fx_effect_falloff_range = 1.0f;
	float render_ssao_fx_radius_multiplier = 2.5f;
	float render_ssao_fx_depth_mip_offset = 3.3f;
	float render_ssao_fx_temporal_stability = 0.25f;
	float render_ssao_fx_spatial_reconstruction_radius = 4.0f;
	float render_ssao_fx_alpha_interpolation = 1.0f;
	bool render_ssao_fx_half_resolution = false;
	bool render_ssao_fx_half_precision_depth = false;
	bool render_ssao_fx_uniform_weighting = false;
	bool render_ssao_fx_reset_accumulation = true;
	bool show_light_icons = true;
	bool light_icon_occlusion = true;
	bool lightmaps_autoloaded = false;
	bool show_fps = false;
};

void ResetViewportPanel(ViewportPanelState& state, const ImVec2& size);
void SyncFlyFromOrbit(ViewportPanelState& state);
void SyncOrbitFromFly(ViewportPanelState& state);
void UpdateViewportControls(ViewportPanelState& state, const ImVec2& origin, const ImVec2& size, bool hovered);
void DrawViewportOverlay(const ViewportPanelState& state, ImDrawList* draw_list, const ImVec2& origin, const ImVec2& size);

/// Returns true if the current view mode is orthographic.
[[nodiscard]] inline bool IsOrthographicView(const ViewportPanelState& state) {
	return state.view_mode != ViewportPanelState::ViewMode::Perspective;
}

/// Sets the view mode and initializes ortho center from current target.
void SetViewMode(ViewportPanelState& state, ViewportPanelState::ViewMode mode);

/// Toggles between perspective and the last-used orthographic view (default: Top).
void ToggleOrthoPerspective(ViewportPanelState& state);

/// Returns display name for view mode.
[[nodiscard]] const char* ViewModeName(ViewportPanelState::ViewMode mode);

/// Focus the viewport on a specific world position.
/// Adjusts orbit target (perspective) or ortho_center (orthographic) and zoom.
/// @param state The viewport state to modify.
/// @param position The world position to focus on (3 floats: x, y, z).
/// @param bounds_min Optional minimum corner of bounding box for auto-zoom.
/// @param bounds_max Optional maximum corner of bounding box for auto-zoom.
void FocusViewportOn(ViewportPanelState& state, const float position[3],
                     const float* bounds_min = nullptr, const float* bounds_max = nullptr);
