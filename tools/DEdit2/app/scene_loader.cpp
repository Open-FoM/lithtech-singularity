#include "app/scene_loader.h"

#include "app/editor_paths.h"
#include "viewport/diligent_viewport.h"
#include "viewport/sky_models.h"
#include "viewport/world_settings.h"

#include "diligent_render.h"
#include "diligent_render_debug.h"
#include "engine_render.h"
#include "ui_viewport.h"
#include "ui_scene.h"

#include <cstdio>

void LoadSceneWorld(
  const std::string& path,
  DiligentContext& diligent,
  const std::string& project_root,
  std::string& world_file,
  ViewportPanelState& viewport_panel,
  std::vector<TreeNode>& scene_nodes,
  std::vector<NodeProperties>& scene_props,
  ScenePanelState& scene_panel,
  std::string& scene_error,
  SelectionTarget& active_target,
  WorldRenderSettingsCache& world_settings_cache)
{
  world_file = path;
  diligent.engine.world_loaded = false;
  viewport_panel.lightmaps_autoloaded = false;
  viewport_panel.render_lightmaps_available = false;
  scene_nodes = BuildSceneTree(world_file, scene_props, scene_error);
  scene_panel.error = scene_error;
  scene_panel.selected_id = scene_nodes.empty() ? -1 : 0;
  scene_panel.tree_ui = {};
  active_target = SelectionTarget::Scene;

  if (const NodeProperties* world_props = FindWorldProperties(scene_props))
  {
    ApplyWorldSettingsToRenderer(*world_props);
    UpdateWorldSettingsCache(*world_props, world_settings_cache);
  }

  const std::string compiled_path = FindCompiledWorldPath(world_file, project_root);
  if (!compiled_path.empty())
  {
    std::string render_error;
    if (!LoadRenderWorld(diligent.engine, compiled_path, render_error))
    {
      std::fprintf(stderr, "World render load failed: %s\n", render_error.c_str());
      diligent.sky_world_models.clear();
      diligent.sky_objects.clear();
    }
    else
    {
      DiligentWorldTextureStats stats{};
      if (diligent_GetWorldTextureStats(stats))
      {
        const bool has_lightmaps = stats.lightmap_present > 0;
        viewport_panel.render_lightmaps_available = has_lightmaps;
        if (has_lightmaps)
        {
          if (!viewport_panel.lightmaps_autoloaded)
          {
            viewport_panel.render_lightmap_mode =
              static_cast<int>(ViewportPanelState::WorldLightmapMode::Baked);
            viewport_panel.lightmaps_autoloaded = true;
          }
        }
        else
        {
          viewport_panel.render_lightmap_mode =
            static_cast<int>(ViewportPanelState::WorldLightmapMode::Dynamic);
          viewport_panel.lightmaps_autoloaded = true;
        }
      }
    }
  }

  // Sync model instances with the scene tree for rendering
  BuildSkyWorldModels(diligent, scene_nodes, scene_props, viewport_panel.sky_world_model);
}
