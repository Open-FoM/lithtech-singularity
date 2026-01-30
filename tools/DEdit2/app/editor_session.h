#pragma once

#include "brush/brush_primitive.h"
#include "document_state.h"
#include "editor_state.h"
#include "multi_viewport.h"
#include "selection/depth_cycle.h"
#include "selection/selection_filter.h"
#include "selection/selection_query.h"
#include "brush/csg_dialogs/csg_dialog_helpers.h"
#include "brush/csg_dialogs/csg_hollow_dialog.h"
#include "brush/csg_dialogs/csg_carve_dialog.h"
#include "brush/csg_dialogs/csg_split_dialog.h"
#include "brush/csg_dialogs/csg_join_dialog.h"
#include "brush/csg_dialogs/csg_triangulate_dialog.h"
#include "brush/geometry_dialogs/geometry_dialog_state.h"
#include "geometry/geometry_mode_state.h"
#include "transform/mirror_dialog.h"
#include "transform/rotate_dialog.h"
#include "ui_marker.h"
#include "undo_stack.h"
#include "ui_console.h"
#include "ui_dock.h"
#include "ui_goto.h"
#include "ui_tools_dock.h"
#include "ui_project.h"
#include "ui_scene.h"
#include "ui_viewport.h"
#include "ui_worlds.h"
#include "viewport/world_settings.h"

#include <string>
#include <vector>

/// State for the primitive creation dialog.
struct PrimitiveDialogState {
  bool open = false;
  PrimitiveType type = PrimitiveType::None;

  // Last-used parameters for each primitive type
  BoxParams box_params;
  CylinderParams cylinder_params;
  PyramidParams pyramid_params;
  SphereParams sphere_params;
  PlaneParams plane_params;
};

/// Drawing plane for manual polygon mode.
enum class PolygonDrawPlane {
  XZ,  ///< Top/bottom view (horizontal plane)
  XY,  ///< Front/back view (vertical plane facing Z)
  YZ,  ///< Left/right view (vertical plane facing X)
};

/// State for manual polygon drawing mode.
struct PolygonDrawState {
  bool active = false;                  ///< Whether polygon drawing mode is active
  std::vector<float> vertices;          ///< Placed vertices (XYZ triplets)
  PolygonDrawPlane plane = PolygonDrawPlane::XZ;  ///< Drawing plane
  float plane_offset = 0.0f;            ///< Offset along plane normal
  float extrusion_height = 64.0f;       ///< Height for extrusion
  bool show_extrusion_dialog = false;   ///< Show dialog to confirm extrusion
  bool is_convex = true;                ///< Convexity check result
};

/// Action that triggered the save prompt dialog.
enum class SavePromptTrigger {
  None,
  NewWorld,
  OpenWorld,
  CloseApplication
};

/// Result from the save prompt dialog.
enum class SavePromptResult {
  Pending,   ///< Dialog still open, no user action yet
  Save,      ///< User chose to save
  DontSave,  ///< User chose not to save
  Cancel     ///< User cancelled the action
};

/// State for the save prompt dialog.
struct SavePromptDialogState {
  bool open = false;
  SavePromptTrigger trigger = SavePromptTrigger::None;
  SavePromptResult result = SavePromptResult::Pending;
  std::string pending_world_path;  ///< World path to open after save prompt (for OpenWorld trigger)
};

/// Panel visibility settings for the editor.
struct PanelVisibility {
  bool show_project = true;
  bool show_worlds = true;
  bool show_scene = true;
  bool show_properties = true;
  bool show_console = true;
  bool show_tools = true;
};

struct EditorSession
{
  std::string project_root;
  std::string world_file;
  std::string project_error;
  std::string scene_error;
  std::vector<std::string> recent_projects;

  /// Panel visibility settings.
  PanelVisibility panel_visibility;

  std::vector<NodeProperties> project_props;
  std::vector<NodeProperties> scene_props;
  std::vector<TreeNode> project_nodes;
  std::vector<TreeNode> scene_nodes;

  ProjectPanelState project_panel;
  WorldBrowserState world_browser;
  ScenePanelState scene_panel;
  ConsolePanelState console_panel;
  MultiViewportState multi_viewport;

  /// Convenience accessor for the active viewport (backward compatible).
  [[nodiscard]] ViewportPanelState& viewport_panel() { return multi_viewport.ActiveViewport(); }
  [[nodiscard]] const ViewportPanelState& viewport_panel() const { return multi_viewport.ActiveViewport(); }

  SelectionTarget active_target = SelectionTarget::Project;
  WorldRenderSettingsCache world_settings_cache;

  UndoStack undo_stack;

  /// Document dirty state tracking.
  DocumentState document_state;

  /// Recent worlds list (separate from recent projects).
  std::vector<std::string> recent_worlds;

  PrimitiveDialogState primitive_dialog;
  ToolsPanelState tools_panel;
  SavePromptDialogState save_prompt_dialog;

  /// Selection filter for controlling which node types can be selected.
  SelectionFilter selection_filter;
  SelectionFilterDialogState selection_filter_dialog;

  /// Depth cycle state for cycling through overlapping objects.
  DepthCycleState depth_cycle;

  /// Advanced selection dialog state.
  AdvancedSelectionDialogState advanced_selection_dialog;

  /// Go To Node dialog state.
  GoToDialogState goto_dialog;

  /// Rotate selection dialog state.
  RotateDialogState rotate_dialog;

  /// Mirror selection dialog state.
  MirrorDialogState mirror_dialog;

  /// Marker position dialog state.
  MarkerDialogState marker_dialog;

  /// Manual polygon drawing mode state.
  PolygonDrawState polygon_draw;

  /// CSG operation dialog states.
  HollowDialogState hollow_dialog;
  CarveDialogState carve_dialog;
  SplitDialogState split_dialog;
  JoinDialogState join_dialog;
  TriangulateDialogState triangulate_dialog;
  CSGErrorPopupState csg_error_popup;

  /// Geometry editing mode state (EPIC-09).
  GeometryModeState geometry_mode;

  /// Geometry operation dialogs (EPIC-09).
  VertexWeldDialogState weld_dialog;
  FaceExtrudeDialogState extrude_dialog;
};
