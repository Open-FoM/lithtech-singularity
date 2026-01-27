#include "viewport/scene_filters.h"

#include "app/string_utils.h"
#include "editor_state.h"
#include "ui_viewport.h"

NodeCategoryFlags ClassifyNodeCategories(const NodeProperties& props)
{
  NodeCategoryFlags flags{};
  const std::string type = LowerCopy(props.type);
  const std::string cls = LowerCopy(props.class_name);
  flags.world = (type == "world") || (cls == "world");
  flags.world_model = (type == "worldmodel") || (type == "world_model") ||
    (cls == "worldmodel") || (cls == "world_model");
  flags.model = !flags.world_model && (type == "model" || cls == "model");
  flags.sprite = (type == "sprite" || cls == "sprite");
  flags.polygrid = (type == "polygrid" || cls == "polygrid" || type == "terrain" || cls == "terrain");
  flags.particles = (type == "particlesystem" || cls == "particlesystem" ||
    type == "particles" || cls == "particles" ||
    type == "particle" || cls == "particle");
  flags.volume = (type == "volumeeffect" || cls == "volumeeffect" ||
    type == "volume" || cls == "volume");
  flags.line_system = (type == "linesystem" || cls == "linesystem" ||
    type == "line" || cls == "line");
  flags.canvas = (type == "canvas" || cls == "canvas");
  flags.sky = (type == "sky" || cls == "sky" ||
    type == "skyobject" || cls == "skyobject");
  return flags;
}

bool NodePickableByRender(const ViewportPanelState& viewport_state, const NodeProperties& props)
{
  const NodeCategoryFlags flags = ClassifyNodeCategories(props);
  if (flags.sky)
  {
    return false;
  }
  if (flags.world && !viewport_state.render_draw_world)
  {
    return false;
  }
  if (flags.world_model && !viewport_state.render_draw_world_models)
  {
    return false;
  }
  if (flags.model && !viewport_state.render_draw_models)
  {
    return false;
  }
  if (flags.sprite && !viewport_state.render_draw_sprites)
  {
    return false;
  }
  if (flags.polygrid && !viewport_state.render_draw_polygrids)
  {
    return false;
  }
  if (flags.particles && !viewport_state.render_draw_particles)
  {
    return false;
  }
  if (flags.volume && !viewport_state.render_draw_volume_effects)
  {
    return false;
  }
  if (flags.line_system && !viewport_state.render_draw_line_systems)
  {
    return false;
  }
  if (flags.canvas && !viewport_state.render_draw_canvases)
  {
    return false;
  }
  if (flags.sky && !viewport_state.render_draw_sky)
  {
    return false;
  }
  return true;
}

bool IsLightNode(const NodeProperties& props)
{
  const std::string type = LowerCopy(props.type);
  const std::string cls = LowerCopy(props.class_name);
  auto is_light = [](const std::string& value) -> bool
  {
    return value == "light" ||
      value == "dirlight" ||
      value == "directionallight" ||
      value == "spotlight" ||
      value == "pointlight";
  };
  return is_light(type) || is_light(cls);
}

bool IsDirectionalLightNode(const NodeProperties& props)
{
  const std::string type = LowerCopy(props.type);
  const std::string cls = LowerCopy(props.class_name);
  auto is_dir = [](const std::string& value) -> bool
  {
    return value == "dirlight" || value == "directionallight";
  };
  return is_dir(type) || is_dir(cls);
}
