#include "viewport/sky_models.h"

#include "app/string_utils.h"
#include "dedit2_concommand.h"
#include "de_objects.h"
#include "editor_state.h"
#include "viewport/diligent_viewport.h"

#include <algorithm>
#include <unordered_map>
#include <unordered_set>

namespace
{
bool IsSkyNode(const NodeProperties& props)
{
  const std::string type = LowerCopy(props.type);
  const std::string cls = LowerCopy(props.class_name);
  return type == "sky" || cls == "sky" || type == "skyobject" || cls == "skyobject";
}

bool IsSkyPointerNode(const NodeProperties& props)
{
  std::string unused;
  return TryGetRawPropertyStringAny(props, {"objectname", "skyobject", "skyobjectname"}, unused);
}

bool SkyTargetMatchesWorldName(const std::string& target, const std::string& world_name)
{
  if (target.empty() || world_name.empty())
  {
    return false;
  }
  const std::string target_lower = LowerCopy(target);
  const std::string name_lower = LowerCopy(world_name);
  return target_lower == name_lower || target_lower == ("world_" + name_lower);
}

std::string GuessSkyWorldModelName(const TreeNode& node, const NodeProperties& props)
{
  std::string out;
  if (TryGetRawPropertyStringAny(
    props,
    {"skyobject", "skyobjectname", "skyobjectname0", "skyobjectname1"},
    out))
  {
    return out;
  }
  if (!props.resource.empty())
  {
    return TrimQuotes(props.resource);
  }
  if (!node.name.empty())
  {
    return TrimQuotes(node.name);
  }
  return {};
}

std::string GuessWorldModelNameForObject(const TreeNode& node, const NodeProperties& props)
{
  std::string out;
  if (TryGetRawPropertyStringAny(
    props,
    {"worldmodel", "worldmodelname", "worldmodelname0", "worldmodelname1"},
    out))
  {
    return out;
  }
  if (!props.resource.empty())
  {
    return TrimQuotes(props.resource);
  }
  if (!node.name.empty())
  {
    return TrimQuotes(node.name);
  }
  return {};
}
}

void BuildSkyWorldModels(
  DiligentContext& ctx,
  const std::vector<TreeNode>& nodes,
  const std::vector<NodeProperties>& props,
  const std::string& selection)
{
  ctx.sky_world_models.clear();
  ctx.sky_objects.clear();
  ctx.sky_world_model_names.clear();

  if (ctx.engine.world_bsp_model_ptrs.empty())
  {
    return;
  }

  std::unordered_map<std::string, std::vector<WorldBsp*>> world_by_name;
  world_by_name.reserve(ctx.engine.world_bsp_model_ptrs.size());
  std::vector<std::string> world_model_names;
  world_model_names.reserve(ctx.engine.world_bsp_model_ptrs.size());
  for (auto* bsp : ctx.engine.world_bsp_model_ptrs)
  {
    if (!bsp)
    {
      continue;
    }
    const std::string name = TrimQuotes(bsp->m_WorldName);
    if (!name.empty())
    {
      world_by_name[LowerCopy(name)].push_back(bsp);
      world_model_names.push_back(name);
    }
  }

  std::unordered_set<WorldBsp*> used;
  used.reserve(world_by_name.size());

  std::unordered_map<std::string, std::string> object_to_world;
  object_to_world.reserve(nodes.size());
  const size_t map_count = std::min(nodes.size(), props.size());
  for (size_t i = 0; i < map_count; ++i)
  {
    if (nodes[i].deleted)
    {
      continue;
    }
    if (nodes[i].name.empty())
    {
      continue;
    }
    const std::string target = GuessWorldModelNameForObject(nodes[i], props[i]);
    if (target.empty())
    {
      continue;
    }
    object_to_world.emplace(LowerCopy(TrimQuotes(nodes[i].name)), LowerCopy(target));
  }

  auto add_world_model = [&](WorldBsp* bsp, const NodeProperties* props_source)
  {
    if (!bsp || used.find(bsp) != used.end())
    {
      return;
    }
    auto instance = std::make_unique<WorldModelInstance>();
    instance->InitWorldData(bsp, nullptr);
    instance->m_Flags |= FLAG_VISIBLE;
    instance->m_Flags2 |= FLAG2_SKYOBJECT;
    if (props_source)
    {
      instance->m_Pos.Init(props_source->position[0], props_source->position[1], props_source->position[2]);
    }
    else
    {
      instance->m_Pos.Init();
    }
    instance->m_Rotation.Init();
    obj_SetupWorldModelTransform(instance.get());
    ctx.sky_objects.push_back(static_cast<LTObject*>(instance.get()));
    ctx.sky_world_models.push_back(std::move(instance));
    used.insert(bsp);
  };

  const size_t count = std::min(nodes.size(), props.size());
  bool has_sky_pointer = false;
  std::vector<std::string> sky_pointer_targets;
  sky_pointer_targets.reserve(count);
  std::unordered_set<std::string> sky_pointer_seen;
  sky_pointer_seen.reserve(count);
  for (size_t i = 0; i < count; ++i)
  {
    if (nodes[i].deleted)
    {
      continue;
    }
    if (!IsSkyPointerNode(props[i]))
    {
      continue;
    }
    has_sky_pointer = true;
    std::string target;
    if (!TryGetRawPropertyStringAny(props[i], {"objectname", "skyobject", "skyobjectname"}, target))
    {
      continue;
    }
    if (!target.empty())
    {
      const std::string normalized = LowerCopy(target);
      if (sky_pointer_seen.insert(normalized).second)
      {
        sky_pointer_targets.push_back(target);
      }
    }
  }
  if (!sky_pointer_targets.empty())
  {
    DEdit2_Log("[Sky] SkyPointer targets: %zu", sky_pointer_targets.size());
    for (const auto& target : sky_pointer_targets)
    {
      DEdit2_Log("[Sky]   target: %s", target.c_str());
    }
  }
  for (const auto& entry : world_by_name)
  {
    if (entry.second.size() > 1)
    {
      DEdit2_Log("[Sky] Duplicate world model '%s': %zu", entry.first.c_str(), entry.second.size());
    }
  }
  ctx.sky_world_model_names = sky_pointer_targets;
  if (ctx.sky_world_model_names.empty())
  {
    ctx.sky_world_model_names = world_model_names;
  }

  auto resolve_target_to_world = [&](const std::string& target) -> WorldBsp*
  {
    for (const auto& name : world_model_names)
    {
      if (SkyTargetMatchesWorldName(target, name))
      {
        auto it = world_by_name.find(LowerCopy(name));
        if (it != world_by_name.end() && !it->second.empty())
        {
          return it->second.front();
        }
      }
    }
    auto it = world_by_name.find(LowerCopy(target));
    if (it != world_by_name.end() && !it->second.empty())
    {
      return it->second.front();
    }
    return nullptr;
  };

  DEdit2_Log("[Sky] Sky objects built: %zu (selection=%s)",
    ctx.sky_objects.size(),
    selection.empty() ? "Auto" : selection.c_str());

  if (!selection.empty())
  {
    if (WorldBsp* selected = resolve_target_to_world(selection))
    {
      ctx.sky_world_models.clear();
      ctx.sky_objects.clear();
      add_world_model(selected, nullptr);
    }
    return;
  }

  if (has_sky_pointer && !sky_pointer_targets.empty())
  {
    if (WorldBsp* selected = resolve_target_to_world(sky_pointer_targets.front()))
    {
      ctx.sky_world_models.clear();
      ctx.sky_objects.clear();
      add_world_model(selected, nullptr);
    }
    return;
  }

  for (size_t i = 0; i < count; ++i)
  {
    if (nodes[i].deleted)
    {
      continue;
    }
    if (!IsSkyNode(props[i]))
    {
      continue;
    }

    std::string target = GuessSkyWorldModelName(nodes[i], props[i]);
    if (!target.empty())
    {
      const std::string target_lower = LowerCopy(target);
      if (WorldBsp* selected = resolve_target_to_world(target))
      {
        add_world_model(selected, &props[i]);
        continue;
      }
      auto ref = object_to_world.find(target_lower);
      if (ref != object_to_world.end())
      {
        if (WorldBsp* selected = resolve_target_to_world(ref->second))
        {
          add_world_model(selected, &props[i]);
          continue;
        }
      }
    }

    std::string object_ref;
    if (TryGetRawPropertyStringAny(props[i], {"objectname", "skyobject", "skyobjectname"}, object_ref))
    {
      const std::string object_lower = LowerCopy(object_ref);
      auto ref = object_to_world.find(object_lower);
      if (ref != object_to_world.end())
      {
        auto obj_it = world_by_name.find(ref->second);
        if (obj_it != world_by_name.end())
        {
          for (auto* bsp : obj_it->second)
          {
            add_world_model(bsp, &props[i]);
          }
          continue;
        }
      }
    }

    if (!target.empty())
    {
      if (WorldBsp* selected = resolve_target_to_world(target))
      {
        add_world_model(selected, &props[i]);
      }
    }
  }

  if (!ctx.sky_objects.empty())
  {
    return;
  }

  for (size_t i = 0; i < count; ++i)
  {
    if (nodes[i].deleted)
    {
      continue;
    }
    const std::string type = LowerCopy(props[i].type);
    const std::string cls = LowerCopy(props[i].class_name);
    const std::string name = LowerCopy(TrimQuotes(nodes[i].name));
    const bool is_translucent_world_model =
      type.find("translucentworldmodel") != std::string::npos ||
      cls.find("translucentworldmodel") != std::string::npos;
    const bool looks_like_sky_name = name.rfind("sl_", 0) == 0;
    if (!is_translucent_world_model && !looks_like_sky_name)
    {
      continue;
    }

    std::string target = GuessWorldModelNameForObject(nodes[i], props[i]);
    if (target.empty())
    {
      target = nodes[i].name;
    }
    if (target.empty())
    {
      continue;
    }
    if (WorldBsp* selected = resolve_target_to_world(target))
    {
      add_world_model(selected, &props[i]);
    }
  }

  if (!ctx.sky_objects.empty())
  {
    return;
  }

  for (const auto& entry : world_by_name)
  {
    if (entry.first.find("sky") == std::string::npos)
    {
      continue;
    }
    if (!entry.second.empty())
    {
      add_world_model(entry.second.front(), nullptr);
    }
  }
}
