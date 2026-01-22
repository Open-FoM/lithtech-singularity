#include "diligent_scene_collect.h"

#include "bdefs.h"
#include "renderstruct.h"
#include "diligent_debug_draw.h"
#include "diligent_internal.h"
#include "diligent_model_draw.h"
#include "diligent_shadow_draw.h"
#include "diligent_world_data.h"
#include "diligent_world_draw.h"
#include "de_objects.h"
#include "ltvector.h"
#include "strtools.h"
#include "viewparams.h"
#include "world_client_bsp.h"
#include "world_tree.h"
#include "../rendererconsolevars.h"

#include <algorithm>
#include <vector>

namespace
{

std::vector<ModelInstance*> g_diligent_translucent_models;
bool g_diligent_collect_translucent_models = false;

float diligent_calc_draw_distance(const LTObject* object, const ViewParams& params)
{
	if (!object)
	{
		return 0.0f;
	}

	if ((object->m_Flags & FLAG_REALLYCLOSE) == 0)
	{
		return (object->GetPos() - params.m_Pos).MagSqr();
	}

	return object->GetPos().MagSqr();
}

template <typename TObject>
void diligent_sort_translucent_list(std::vector<TObject*>& objects, const ViewParams& params)
{
	if (!g_CV_DrawSorted.m_Val || objects.size() < 2)
	{
		return;
	}

	std::sort(objects.begin(), objects.end(), [&params](const TObject* a, const TObject* b)
	{
		if (!a || !b)
		{
			return a != nullptr;
		}

		const auto* obj_a = static_cast<const LTObject*>(a);
		const auto* obj_b = static_cast<const LTObject*>(b);
		const bool a_close = (obj_a->m_Flags & FLAG_REALLYCLOSE) != 0;
		const bool b_close = (obj_b->m_Flags & FLAG_REALLYCLOSE) != 0;
		if (a_close != b_close)
		{
			return !a_close && b_close;
		}

		const float dist_a = diligent_calc_draw_distance(obj_a, params);
		const float dist_b = diligent_calc_draw_distance(obj_b, params);
		return dist_a > dist_b;
	});
}

void diligent_process_model_attachments(LTObject* object, uint32 depth)
{
	if (!object || depth >= 32 || !g_diligent_state.render_struct || !g_diligent_state.render_struct->ProcessAttachment)
	{
		return;
	}

	Attachment* current = object->m_Attachments;
	while (current)
	{
		LTObject* attached = g_diligent_state.render_struct->ProcessAttachment(object, current);
		if (attached && attached->m_WTFrameCode != g_diligent_state.object_frame_code)
		{
			if (attached->m_Attachments)
			{
				diligent_process_model_attachments(attached, depth + 1);
			}

			if (diligent_should_process_model(attached))
			{
				attached->m_WTFrameCode = g_diligent_state.object_frame_code;
				diligent_draw_model_instance(attached->ToModel());
			}
		}

		current = current->m_pNext;
	}
}

void diligent_process_model_object(LTObject* object)
{
	if (!diligent_should_process_model(object))
	{
		return;
	}

	object->m_WTFrameCode = g_diligent_state.object_frame_code;
	diligent_debug_add_model_info(object->ToModel());
	if (!g_diligent_shadow_mode && !g_diligent_state.glow_mode && g_CV_ModelShadow_Proj_Enable.m_Val)
	{
		ModelInstance* instance = object->ToModel();
		if (instance)
		{
			diligent_queue_model_shadows(instance);
		}
	}

	if (g_diligent_collect_translucent_models && object->IsTranslucent())
	{
		auto* instance = object->ToModel();
		if (instance)
		{
			g_diligent_translucent_models.push_back(instance);
		}
		return;
	}

	diligent_draw_model_instance(object->ToModel());
	if (object->m_Attachments)
	{
		diligent_process_model_attachments(object, 0);
	}
}

bool diligent_should_process_world_model(LTObject* object)
{
	if (!object || !object->HasWorldModel())
	{
		return false;
	}

	if (!g_CV_DrawWorldModels.m_Val)
	{
		return false;
	}

	if ((object->m_Flags & FLAG_VISIBLE) == 0)
	{
		return false;
	}

	if (object->m_Flags2 & FLAG2_SKYOBJECT)
	{
		return false;
	}

	if (object->m_WTFrameCode == g_diligent_state.object_frame_code)
	{
		return false;
	}

	return true;
}

void diligent_process_world_model_object(LTObject* object)
{
	if (!diligent_should_process_world_model(object))
	{
		return;
	}

	object->m_WTFrameCode = g_diligent_state.object_frame_code;

	auto* instance = object->ToWorldModel();
	if (!instance)
	{
		return;
	}

	if (object->IsTranslucent())
	{
		g_diligent_translucent_world_models.push_back(instance);
	}
	else
	{
		g_diligent_solid_world_models.push_back(instance);
	}
}

void diligent_process_light_object(LTObject* object)
{
	if (!object || object->m_ObjectType != OT_LIGHT)
	{
		return;
	}

	if (!g_CV_DynamicLight.m_Val)
	{
		return;
	}

	DynamicLight* light = object->ToDynamicLight();
	if (!light)
	{
		return;
	}

	if (!g_CV_DynamicLightWorld.m_Val && ((light->m_Flags2 & FLAG2_FORCEDYNAMICLIGHTWORLD) == 0))
	{
		return;
	}

	if (g_diligent_num_world_dynamic_lights >= kDiligentMaxDynamicLights)
	{
		return;
	}

	g_diligent_world_dynamic_lights[g_diligent_num_world_dynamic_lights++] = light;
}

void diligent_collect_world_dynamic_lights(const ViewParams& params, WorldTreeNode* node)
{
	if (!node)
	{
		return;
	}

	auto* list_head = node->m_Objects[NOA_Objects].AsDLink();
	for (LTLink* current = list_head->m_pNext; current != list_head; current = current->m_pNext)
	{
		auto* object = static_cast<LTObject*>(current->m_pData);
		if (!object || object->m_ObjectType != OT_LIGHT)
		{
			continue;
		}

		const LTVector min = object->m_Pos - object->m_Dims;
		const LTVector max = object->m_Pos + object->m_Dims;
		if (!params.ViewAABBIntersect(min, max))
		{
			continue;
		}

		diligent_process_light_object(object);
	}

	if (!node->HasChildren())
	{
		return;
	}

	for (uint32 child_index = 0; child_index < MAX_WTNODE_CHILDREN; ++child_index)
	{
		WorldTreeNode* child = node->GetChild(child_index);
		if (!child || child->GetNumObjectsOnOrBelow() == 0)
		{
			continue;
		}

		if (!params.ViewAABBIntersect(child->GetBBoxMin(), child->GetBBoxMax()))
		{
			continue;
		}

		diligent_collect_world_dynamic_lights(params, child);
	}
}

void diligent_filter_world_node_for_models(const ViewParams& params, WorldTreeNode* node)
{
	if (!node)
	{
		return;
	}

	auto* list_head = node->m_Objects[NOA_Objects].AsDLink();
	for (LTLink* current = list_head->m_pNext; current != list_head; current = current->m_pNext)
	{
		auto* object = static_cast<LTObject*>(current->m_pData);
		if (!object)
		{
			continue;
		}

		const LTVector min = object->m_Pos - object->m_Dims;
		const LTVector max = object->m_Pos + object->m_Dims;
		if (!params.ViewAABBIntersect(min, max))
		{
			continue;
		}

		diligent_process_model_object(object);
	}

	if (!node->HasChildren())
	{
		return;
	}

	for (uint32 child_index = 0; child_index < MAX_WTNODE_CHILDREN; ++child_index)
	{
		WorldTreeNode* child = node->GetChild(child_index);
		if (!child || child->GetNumObjectsOnOrBelow() == 0)
		{
			continue;
		}

		if (!params.ViewAABBIntersect(child->GetBBoxMin(), child->GetBBoxMax()))
		{
			continue;
		}

		diligent_filter_world_node_for_models(params, child);
	}
}

void diligent_filter_world_node_for_world_models(const ViewParams& params, WorldTreeNode* node)
{
	if (!node)
	{
		return;
	}

	auto* list_head = node->m_Objects[NOA_Objects].AsDLink();
	for (LTLink* current = list_head->m_pNext; current != list_head; current = current->m_pNext)
	{
		auto* object = static_cast<LTObject*>(current->m_pData);
		if (!object || !object->HasWorldModel())
		{
			continue;
		}

		const LTVector min = object->m_Pos - object->m_Dims;
		const LTVector max = object->m_Pos + object->m_Dims;
		if (!params.ViewAABBIntersect(min, max))
		{
			continue;
		}

		diligent_process_world_model_object(object);
	}

	if (!node->HasChildren())
	{
		return;
	}

	for (uint32 child_index = 0; child_index < MAX_WTNODE_CHILDREN; ++child_index)
	{
		WorldTreeNode* child = node->GetChild(child_index);
		if (!child || child->GetNumObjectsOnOrBelow() == 0)
		{
			continue;
		}

		if (!params.ViewAABBIntersect(child->GetBBoxMin(), child->GetBBoxMax()))
		{
			continue;
		}

		diligent_filter_world_node_for_world_models(params, child);
	}
}

} // namespace

DiligentRenderWorld* diligent_find_world_model(const WorldModelInstance* instance)
{
	if (!instance || !g_render_world)
	{
		return nullptr;
	}

	const auto* bsp = instance->GetOriginalBsp();
	if (!bsp)
	{
		return nullptr;
	}

	const uint32 name_hash = st_GetHashCode(bsp->m_WorldName);
	auto it = g_render_world->world_models.find(name_hash);
	return it != g_render_world->world_models.end() ? it->second.get() : nullptr;
}

bool diligent_should_process_model(LTObject* object)
{
	if (!object || !object->IsModel())
	{
		return false;
	}

	if (!g_CV_DrawModels.m_Val)
	{
		return false;
	}

	if ((object->m_Flags & FLAG_VISIBLE) == 0)
	{
		return false;
	}

	if (object->m_Flags2 & FLAG2_SKYOBJECT)
	{
		return false;
	}

	if (object->m_WTFrameCode == g_diligent_state.object_frame_code)
	{
		return false;
	}

	const bool translucent = object->IsTranslucent();
	if (translucent && !g_CV_DrawTranslucentModels.m_Val)
	{
		return false;
	}
	if (!translucent && !g_CV_DrawSolidModels.m_Val)
	{
		return false;
	}

	return true;
}

bool diligent_draw_models(SceneDesc* desc)
{
	if (!desc)
	{
		return true;
	}
	if (!g_CV_DrawModels.m_Val)
	{
		return true;
	}

	diligent_clear_shadow_queue();
	g_diligent_translucent_models.clear();
	g_diligent_collect_translucent_models = (g_CV_DrawSorted.m_Val != 0) && (g_CV_DrawTranslucentModels.m_Val != 0);

	if (desc->m_DrawMode == DRAWMODE_OBJECTLIST)
	{
		if (!desc->m_pObjectList || desc->m_ObjectListSize <= 0)
		{
			g_diligent_collect_translucent_models = false;
			return true;
		}

		if (g_diligent_state.render_struct && g_diligent_state.render_struct->IncObjectFrameCode)
		{
			g_diligent_state.object_frame_code = g_diligent_state.render_struct->IncObjectFrameCode();
		}
		else
		{
			++g_diligent_state.object_frame_code;
		}

		for (int i = 0; i < desc->m_ObjectListSize; ++i)
		{
			LTObject* object = desc->m_pObjectList[i];
			if (!object)
			{
				continue;
			}

			diligent_process_model_object(object);
		}
	}
	else
	{
		if (desc->m_DrawMode != DRAWMODE_NORMAL)
		{
			g_diligent_collect_translucent_models = false;
			return true;
		}

		auto* bsp_client = diligent_get_world_bsp_client();
		if (!bsp_client)
		{
			g_diligent_collect_translucent_models = false;
			return true;
		}

		WorldTree* tree = bsp_client->ClientTree();
		if (!tree)
		{
			g_diligent_collect_translucent_models = false;
			return true;
		}

		if (g_diligent_state.render_struct && g_diligent_state.render_struct->IncObjectFrameCode)
		{
			g_diligent_state.object_frame_code = g_diligent_state.render_struct->IncObjectFrameCode();
		}
		else
		{
			++g_diligent_state.object_frame_code;
		}

		diligent_filter_world_node_for_models(g_diligent_state.view_params, tree->GetRootNode());

		auto* list_head = tree->m_AlwaysVisObjects.AsDLink();
		for (LTLink* current = list_head->m_pNext; current != list_head; current = current->m_pNext)
		{
			auto* object = static_cast<LTObject*>(current->m_pData);
			if (!object)
			{
				continue;
			}

			diligent_process_model_object(object);
		}
	}

	g_diligent_collect_translucent_models = false;
	if (!g_diligent_translucent_models.empty())
	{
		diligent_sort_translucent_list(g_diligent_translucent_models, g_diligent_state.view_params);
		for (auto* instance : g_diligent_translucent_models)
		{
			if (!instance)
			{
				continue;
			}

			diligent_draw_model_instance(instance);
			if (instance->m_Attachments)
			{
				diligent_process_model_attachments(instance, 0);
			}
		}
	}

	diligent_render_queued_model_shadows(g_diligent_state.view_params);
	return true;
}

void diligent_collect_world_models(SceneDesc* desc)
{
	g_diligent_solid_world_models.clear();
	g_diligent_translucent_world_models.clear();

	if (!desc || !g_CV_DrawWorldModels.m_Val)
	{
		return;
	}

	if (g_diligent_state.render_struct && g_diligent_state.render_struct->IncObjectFrameCode)
	{
		g_diligent_state.object_frame_code = g_diligent_state.render_struct->IncObjectFrameCode();
	}
	else
	{
		++g_diligent_state.object_frame_code;
	}

	if (desc->m_DrawMode == DRAWMODE_OBJECTLIST)
	{
		if (!desc->m_pObjectList || desc->m_ObjectListSize <= 0)
		{
			return;
		}

		for (int i = 0; i < desc->m_ObjectListSize; ++i)
		{
			LTObject* object = desc->m_pObjectList[i];
			if (!object)
			{
				continue;
			}

			diligent_process_world_model_object(object);
		}

		return;
	}

	if (desc->m_DrawMode != DRAWMODE_NORMAL)
	{
		return;
	}

	auto* bsp_client = diligent_get_world_bsp_client();
	if (!bsp_client)
	{
		return;
	}

	WorldTree* tree = bsp_client->ClientTree();
	if (!tree)
	{
		return;
	}

	diligent_filter_world_node_for_world_models(g_diligent_state.view_params, tree->GetRootNode());

	auto* list_head = tree->m_AlwaysVisObjects.AsDLink();
	for (LTLink* current = list_head->m_pNext; current != list_head; current = current->m_pNext)
	{
		auto* object = static_cast<LTObject*>(current->m_pData);
		if (!object)
		{
			continue;
		}

		diligent_process_world_model_object(object);
	}
}

void diligent_collect_visible_render_blocks(const ViewParams& params)
{
	g_visible_render_blocks.clear();
	if (!g_render_world)
	{
		return;
	}

	g_visible_render_blocks.reserve(g_render_world->render_blocks.size());
	for (const auto& block : g_render_world->render_blocks)
	{
		if (!block)
		{
			continue;
		}

		if (params.ViewAABBIntersect(block->bounds_min, block->bounds_max))
		{
			g_visible_render_blocks.push_back(block.get());
		}
	}
}

void diligent_collect_scene_dynamic_lights(SceneDesc* desc, const ViewParams& params)
{
	g_diligent_num_world_dynamic_lights = 0;
	if (!desc)
	{
		return;
	}

	if (desc->m_DrawMode == DRAWMODE_OBJECTLIST)
	{
		if (desc->m_pObjectList && desc->m_ObjectListSize > 0)
		{
			for (int i = 0; i < desc->m_ObjectListSize; ++i)
			{
				LTObject* object = desc->m_pObjectList[i];
				if (!object)
				{
					continue;
				}

				diligent_process_light_object(object);
			}
		}
	}
	else if (desc->m_DrawMode == DRAWMODE_NORMAL)
	{
		auto* bsp_client = diligent_get_world_bsp_client();
		if (!bsp_client)
		{
			return;
		}

		WorldTree* tree = bsp_client->ClientTree();
		if (!tree)
		{
			return;
		}

		diligent_collect_world_dynamic_lights(params, tree->GetRootNode());

		auto* list_head = tree->m_AlwaysVisObjects.AsDLink();
		for (LTLink* current = list_head->m_pNext; current != list_head; current = current->m_pNext)
		{
			auto* object = static_cast<LTObject*>(current->m_pData);
			if (!object)
			{
				continue;
			}

			diligent_process_light_object(object);
		}
	}
}
