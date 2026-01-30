#include "csg_dialog_helpers.h"

#include "brush/texture_ops/uv_types.h"

#include "imgui.h"

bool ExtractBrushGeometry(const NodeProperties& props, std::vector<float>& out_vertices,
                          std::vector<uint32_t>& out_indices) {
  if (props.type != "Brush") {
    return false;
  }
  if (props.brush_vertices.empty() || props.brush_indices.empty()) {
    return false;
  }
  out_vertices = props.brush_vertices;
  out_indices = props.brush_indices;
  return true;
}

bool IsValidCSGBrush(const TreeNode& node, const NodeProperties& props) {
  if (node.deleted || node.is_folder) {
    return false;
  }
  if (props.type != "Brush") {
    return false;
  }
  if (props.brush_vertices.empty() || props.brush_indices.empty()) {
    return false;
  }
  return true;
}

std::vector<int> GetSelectedCSGBrushIds(const ScenePanelState& state, const std::vector<TreeNode>& nodes,
                                        const std::vector<NodeProperties>& props) {
  std::vector<int> result;
  for (int id : state.selected_ids) {
    if (id < 0 || static_cast<size_t>(id) >= nodes.size()) {
      continue;
    }
    if (IsValidCSGBrush(nodes[id], props[id])) {
      result.push_back(id);
    }
  }
  return result;
}

int CreateBrushFromCSGResult(std::vector<TreeNode>& scene_nodes, std::vector<NodeProperties>& scene_props,
                             const std::vector<float>& vertices, const std::vector<uint32_t>& indices,
                             const char* name) {
  if (vertices.empty() || indices.empty()) {
    return -1;
  }

  // Find or create root node
  if (scene_nodes.empty()) {
    TreeNode root;
    root.name = "World";
    root.is_folder = true;
    scene_nodes.push_back(root);
    scene_props.push_back(MakeProps("World"));
  }

  const int new_id = static_cast<int>(scene_nodes.size());
  TreeNode node;
  node.name = name;
  node.is_folder = false;
  scene_nodes.push_back(node);

  NodeProperties props = MakeProps("Brush");
  props.brush_vertices = vertices;
  props.brush_indices = indices;

  // Initialize brush_face_textures for each triangle face
  // Each face is a triangle, so face_count = indices.size() / 3
  const size_t face_count = indices.size() / 3;
  props.brush_face_textures.resize(face_count);
  for (auto& face : props.brush_face_textures) {
    face.texture_name = "default";
    face.surface_flags = static_cast<uint32_t>(texture_ops::SurfaceFlags::Solid);
  }

  // Compute centroid from vertices
  if (vertices.size() >= 3) {
    const size_t vertex_count = vertices.size() / 3;
    double sum_x = 0.0, sum_y = 0.0, sum_z = 0.0;
    for (size_t i = 0; i < vertices.size(); i += 3) {
      sum_x += vertices[i];
      sum_y += vertices[i + 1];
      sum_z += vertices[i + 2];
    }
    props.position[0] = static_cast<float>(sum_x / static_cast<double>(vertex_count));
    props.position[1] = static_cast<float>(sum_y / static_cast<double>(vertex_count));
    props.position[2] = static_cast<float>(sum_z / static_cast<double>(vertex_count));
  }

  scene_props.push_back(props);

  // Add to root's children
  scene_nodes[0].children.push_back(new_id);

  return new_id;
}

void DeleteBrushNodes(const std::vector<int>& node_ids, std::vector<TreeNode>& scene_nodes, UndoStack* undo_stack) {
  for (int id : node_ids) {
    if (id < 0 || static_cast<size_t>(id) >= scene_nodes.size()) {
      continue;
    }
    if (undo_stack != nullptr) {
      undo_stack->PushDelete(UndoTarget::Scene, id, scene_nodes[id].deleted);
    }
    scene_nodes[id].deleted = true;
  }
}

void DrawCSGErrorPopup(CSGErrorPopupState& error_state) {
  if (!error_state.show) {
    return;
  }

  ImGui::OpenPopup("CSG Operation Error");

  ImVec2 center = ImGui::GetMainViewport()->GetCenter();
  ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

  if (ImGui::BeginPopupModal("CSG Operation Error", &error_state.show, ImGuiWindowFlags_AlwaysAutoResize)) {
    ImGui::TextWrapped("%s", error_state.message.c_str());
    ImGui::Separator();
    if (ImGui::Button("OK", ImVec2(120, 0))) {
      error_state.show = false;
      ImGui::CloseCurrentPopup();
    }
    ImGui::EndPopup();
  }
}
