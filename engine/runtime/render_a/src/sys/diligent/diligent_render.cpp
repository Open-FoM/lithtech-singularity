#include "bdefs.h"
#include "renderstruct.h"
#include "ltbasedefs.h"
#include "ltbasetypes.h"
#include "pixelformat.h"
#include "dtxmgr.h"

#ifdef LTJS_SDL_BACKEND
#include "ltjs_main_window_descriptor.h"
#endif

#include "Common/interface/RefCntAutoPtr.hpp"
#include "Graphics/GraphicsEngine/interface/DeviceContext.h"
#include "Graphics/GraphicsEngine/interface/GraphicsTypes.h"
#include "Graphics/GraphicsEngine/interface/Buffer.h"
#include "Graphics/GraphicsEngine/interface/PipelineState.h"
#include "Graphics/GraphicsEngine/interface/RenderDevice.h"
#include "Graphics/GraphicsEngine/interface/Shader.h"
#include "Graphics/GraphicsEngine/interface/ShaderResourceBinding.h"
#include "Graphics/GraphicsEngine/interface/SwapChain.h"
#include "Graphics/GraphicsEngineVulkan/interface/EngineFactoryVk.h"
#include "ltrenderstyle.h"

#include <array>
#include <cstring>
#include <unordered_map>
#include <vector>

#ifndef __WINDOWS_H__
#include <windows.h>
#define __WINDOWS_H__
#endif

namespace
{
struct DiligentSurface
{
	uint32 width;
	uint32 height;
	uint32 pitch;
	uint32 data[1];
};

Diligent::RefCntAutoPtr<Diligent::IRenderDevice> g_render_device;
Diligent::RefCntAutoPtr<Diligent::IDeviceContext> g_immediate_context;
Diligent::RefCntAutoPtr<Diligent::ISwapChain> g_swap_chain;
RenderStruct* g_render_struct = nullptr;
uint32 g_screen_width = 0;
uint32 g_screen_height = 0;
bool g_is_in_3d = false;
bool g_is_in_optimized_2d = false;
Diligent::IEngineFactoryVk* g_engine_factory = nullptr;
HWND g_native_window_handle = nullptr;
#ifdef LTJS_SDL_BACKEND
SDL_Window* g_sdl_window = nullptr;
#endif

struct DiligentPsoKey
{
	uint64 render_pass_hash = 0;
	uint32 input_layout_hash = 0;
	int vertex_shader_id = 0;
	int pixel_shader_id = 0;
	uint32 color_format = 0;
	uint32 depth_format = 0;
	uint32 topology = 0;

	bool operator==(const DiligentPsoKey& other) const
	{
		return render_pass_hash == other.render_pass_hash &&
			input_layout_hash == other.input_layout_hash &&
			vertex_shader_id == other.vertex_shader_id &&
			pixel_shader_id == other.pixel_shader_id &&
			color_format == other.color_format &&
			depth_format == other.depth_format &&
			topology == other.topology;
	}
};

uint64 diligent_hash_combine(uint64 seed, uint64 value)
{
	return seed ^ (value + 0x9e3779b97f4a7c15ULL + (seed << 6) + (seed >> 2));
}

uint32 diligent_float_bits(float value)
{
	uint32 bits = 0;
	std::memcpy(&bits, &value, sizeof(bits));
	return bits;
}

uint64 diligent_hash_texture_stage(const TextureStageOps& stage)
{
	uint64 hash = 0;
	hash = diligent_hash_combine(hash, static_cast<uint64>(stage.TextureParam));
	hash = diligent_hash_combine(hash, static_cast<uint64>(stage.ColorOp));
	hash = diligent_hash_combine(hash, static_cast<uint64>(stage.ColorArg1));
	hash = diligent_hash_combine(hash, static_cast<uint64>(stage.ColorArg2));
	hash = diligent_hash_combine(hash, static_cast<uint64>(stage.AlphaOp));
	hash = diligent_hash_combine(hash, static_cast<uint64>(stage.AlphaArg1));
	hash = diligent_hash_combine(hash, static_cast<uint64>(stage.AlphaArg2));
	hash = diligent_hash_combine(hash, static_cast<uint64>(stage.UVSource));
	hash = diligent_hash_combine(hash, static_cast<uint64>(stage.UAddress));
	hash = diligent_hash_combine(hash, static_cast<uint64>(stage.VAddress));
	hash = diligent_hash_combine(hash, static_cast<uint64>(stage.TexFilter));
	hash = diligent_hash_combine(hash, stage.UVTransform_Enable ? 1u : 0u);
	hash = diligent_hash_combine(hash, stage.ProjectTexCoord ? 1u : 0u);
	hash = diligent_hash_combine(hash, stage.TexCoordCount);
	return hash;
}

uint64 diligent_hash_render_pass(const RenderPassOp& pass)
{
	uint64 hash = 0;
	for (uint32 i = 0; i < 4; ++i)
	{
		hash = diligent_hash_combine(hash, diligent_hash_texture_stage(pass.TextureStages[i]));
	}

	hash = diligent_hash_combine(hash, static_cast<uint64>(pass.BlendMode));
	hash = diligent_hash_combine(hash, static_cast<uint64>(pass.ZBufferMode));
	hash = diligent_hash_combine(hash, static_cast<uint64>(pass.CullMode));
	hash = diligent_hash_combine(hash, pass.TextureFactor);
	hash = diligent_hash_combine(hash, pass.AlphaRef);
	hash = diligent_hash_combine(hash, pass.DynamicLight ? 1u : 0u);
	hash = diligent_hash_combine(hash, static_cast<uint64>(pass.ZBufferTestMode));
	hash = diligent_hash_combine(hash, static_cast<uint64>(pass.AlphaTestMode));
	hash = diligent_hash_combine(hash, static_cast<uint64>(pass.FillMode));
	hash = diligent_hash_combine(hash, pass.bUseBumpEnvMap ? 1u : 0u);
	hash = diligent_hash_combine(hash, pass.BumpEnvMapStage);
	hash = diligent_hash_combine(hash, diligent_float_bits(pass.fBumpEnvMap_Scale));
	hash = diligent_hash_combine(hash, diligent_float_bits(pass.fBumpEnvMap_Offset));
	return hash;
}

DiligentPsoKey diligent_make_pso_key(
	const RenderPassOp& pass,
	const RSD3DRenderPass& d3d_pass,
	uint32 input_layout_hash,
	Diligent::TEXTURE_FORMAT color_format,
	Diligent::TEXTURE_FORMAT depth_format,
	Diligent::PRIMITIVE_TOPOLOGY topology)
{
	DiligentPsoKey key;
	key.render_pass_hash = diligent_hash_render_pass(pass);
	key.input_layout_hash = input_layout_hash;
	key.vertex_shader_id = d3d_pass.VertexShaderID;
	key.pixel_shader_id = d3d_pass.PixelShaderID;
	key.color_format = static_cast<uint32>(color_format);
	key.depth_format = static_cast<uint32>(depth_format);
	key.topology = static_cast<uint32>(topology);
	return key;
}

struct DiligentPsoKeyHash
{
	size_t operator()(const DiligentPsoKey& key) const noexcept
	{
		uint64 hash = 0;
		hash = diligent_hash_combine(hash, key.render_pass_hash);
		hash = diligent_hash_combine(hash, key.input_layout_hash);
		hash = diligent_hash_combine(hash, static_cast<uint64>(key.vertex_shader_id));
		hash = diligent_hash_combine(hash, static_cast<uint64>(key.pixel_shader_id));
		hash = diligent_hash_combine(hash, key.color_format);
		hash = diligent_hash_combine(hash, key.depth_format);
		hash = diligent_hash_combine(hash, key.topology);
		return static_cast<size_t>(hash);
	}
};

class DiligentPsoCache
{
public:
	Diligent::RefCntAutoPtr<Diligent::IPipelineState> GetOrCreate(
		const DiligentPsoKey& key,
		const Diligent::GraphicsPipelineStateCreateInfo& create_info)
	{
		if (!g_render_device)
		{
			return {};
		}

		auto it = cache_.find(key);
		if (it != cache_.end())
		{
			return it->second;
		}

		Diligent::RefCntAutoPtr<Diligent::IPipelineState> pipeline_state;
		g_render_device->CreatePipelineState(create_info, &pipeline_state);
		if (pipeline_state)
		{
			cache_.emplace(key, pipeline_state);
		}

		return pipeline_state;
	}

	void Reset()
	{
		cache_.clear();
	}

private:
	std::unordered_map<DiligentPsoKey, Diligent::RefCntAutoPtr<Diligent::IPipelineState>, DiligentPsoKeyHash> cache_;
};

DiligentPsoCache g_pso_cache;

using DiligentTextureArray = std::array<SharedTexture*, 4>;

DiligentTextureArray diligent_resolve_textures(const RenderPassOp& pass, SharedTexture** texture_list)
{
	DiligentTextureArray textures{};
	if (!texture_list)
	{
		return textures;
	}

	for (uint32 stage_index = 0; stage_index < 4; ++stage_index)
	{
		switch (pass.TextureStages[stage_index].TextureParam)
		{
			case RENDERSTYLE_USE_TEXTURE1:
				textures[stage_index] = texture_list[0];
				break;
			case RENDERSTYLE_USE_TEXTURE2:
				textures[stage_index] = texture_list[1];
				break;
			case RENDERSTYLE_USE_TEXTURE3:
				textures[stage_index] = texture_list[2];
				break;
			case RENDERSTYLE_USE_TEXTURE4:
				textures[stage_index] = texture_list[3];
				break;
			default:
				textures[stage_index] = nullptr;
				break;
		}
	}

	return textures;
}

struct DiligentSrbKey
{
	const Diligent::IPipelineState* pipeline_state = nullptr;
	DiligentTextureArray textures{};

	bool operator==(const DiligentSrbKey& other) const
	{
		return pipeline_state == other.pipeline_state && textures == other.textures;
	}
};

struct DiligentSrbKeyHash
{
	size_t operator()(const DiligentSrbKey& key) const noexcept
	{
		uint64 hash = 0;
		hash = diligent_hash_combine(hash, static_cast<uint64>(reinterpret_cast<uintptr_t>(key.pipeline_state)));
		for (const auto* texture : key.textures)
		{
			hash = diligent_hash_combine(hash, static_cast<uint64>(reinterpret_cast<uintptr_t>(texture)));
		}
		return static_cast<size_t>(hash);
	}
};

class DiligentSrbCache
{
public:
	Diligent::RefCntAutoPtr<Diligent::IShaderResourceBinding> GetOrCreate(
		Diligent::IPipelineState* pipeline_state,
		const DiligentTextureArray& textures)
	{
		if (!pipeline_state)
		{
			return {};
		}

		DiligentSrbKey key;
		key.pipeline_state = pipeline_state;
		key.textures = textures;

		auto it = cache_.find(key);
		if (it != cache_.end())
		{
			return it->second;
		}

		Diligent::RefCntAutoPtr<Diligent::IShaderResourceBinding> srb;
		pipeline_state->CreateShaderResourceBinding(&srb, true);
		if (srb)
		{
			cache_.emplace(key, srb);
		}

		return srb;
	}

	void Reset()
	{
		cache_.clear();
	}

private:
	std::unordered_map<DiligentSrbKey, Diligent::RefCntAutoPtr<Diligent::IShaderResourceBinding>, DiligentSrbKeyHash> cache_;
};

DiligentSrbCache g_srb_cache;

struct DiligentRenderTexture
{
	Diligent::RefCntAutoPtr<Diligent::ITexture> texture;
	Diligent::RefCntAutoPtr<Diligent::ITextureView> srv;
	TextureData* texture_data = nullptr;
	uint32 width = 0;
	uint32 height = 0;
	BPPIdent format = BPP_32;
};

SharedTexture* g_drawprim_texture = nullptr;

Diligent::TEXTURE_FORMAT diligent_map_texture_format(BPPIdent format)
{
	switch (format)
	{
		case BPP_32:
			return Diligent::TEX_FORMAT_RGBA8_UNORM;
		case BPP_24:
			return Diligent::TEX_FORMAT_RGB8_UNORM;
		case BPP_16:
			return Diligent::TEX_FORMAT_B5G6R5_UNORM;
		case BPP_S3TC_DXT1:
			return Diligent::TEX_FORMAT_BC1_UNORM;
		case BPP_S3TC_DXT3:
			return Diligent::TEX_FORMAT_BC2_UNORM;
		case BPP_S3TC_DXT5:
			return Diligent::TEX_FORMAT_BC3_UNORM;
		default:
			return Diligent::TEX_FORMAT_UNKNOWN;
	}
}

uint32 diligent_get_mip_count(const TextureData* texture_data)
{
	if (!texture_data)
	{
		return 0;
	}

	uint32 mip_count = texture_data->m_Header.m_nMipmaps;
	if (mip_count == 0)
	{
		mip_count = 1;
	}

	return LTMIN(mip_count, static_cast<uint32>(MAX_DTX_MIPMAPS));
}

void diligent_release_render_texture(SharedTexture* shared_texture)
{
	if (!shared_texture)
	{
		return;
	}

	auto* render_texture = static_cast<DiligentRenderTexture*>(shared_texture->m_pRenderData);
	if (!render_texture)
	{
		return;
	}

	delete render_texture;
	shared_texture->m_pRenderData = nullptr;
}

DiligentRenderTexture* diligent_get_render_texture(SharedTexture* shared_texture, bool texture_changed)
{
	if (!shared_texture)
	{
		return nullptr;
	}

	auto* render_texture = static_cast<DiligentRenderTexture*>(shared_texture->m_pRenderData);
	if (render_texture && !texture_changed)
	{
		return render_texture;
	}

	diligent_release_render_texture(shared_texture);

	if (!g_render_struct || !g_render_device)
	{
		return nullptr;
	}

	TextureData* texture_data = g_render_struct->GetTexture(shared_texture);
	if (!texture_data)
	{
		return nullptr;
	}

	const auto mip_count = diligent_get_mip_count(texture_data);
	if (mip_count == 0)
	{
		return nullptr;
	}

	const auto format = diligent_map_texture_format(texture_data->m_Header.GetBPPIdent());
	if (format == Diligent::TEX_FORMAT_UNKNOWN)
	{
		return nullptr;
	}

	const auto& base_mip = texture_data->m_Mips[0];
	if (base_mip.m_Width == 0 || base_mip.m_Height == 0)
	{
		return nullptr;
	}

	auto* new_render_texture = new DiligentRenderTexture();
	new_render_texture->texture_data = texture_data;
	new_render_texture->format = texture_data->m_Header.GetBPPIdent();

	Diligent::TextureDesc desc;
	desc.Name = "ltjs_shared_texture";
	desc.Type = Diligent::RESOURCE_DIM_TEX_2D;
	desc.Width = base_mip.m_Width;
	desc.Height = base_mip.m_Height;
	desc.MipLevels = mip_count;
	desc.Format = format;
	desc.Usage = Diligent::USAGE_DEFAULT;
	desc.BindFlags = Diligent::BIND_SHADER_RESOURCE;

	std::vector<Diligent::TextureSubResData> subresources;
	subresources.reserve(mip_count);
	for (uint32 level = 0; level < mip_count; ++level)
	{
		const auto& mip = texture_data->m_Mips[level];
		if (!mip.m_Data)
		{
			break;
		}

		Diligent::TextureSubResData subresource;
		subresource.pData = mip.m_Data;
		subresource.Stride = static_cast<Diligent::Uint64>(mip.m_Pitch);
		subresource.DepthStride = 0;
		subresources.push_back(subresource);
	}

	Diligent::TextureData init_data{
		subresources.data(),
		static_cast<Diligent::Uint32>(subresources.size())};

	g_render_device->CreateTexture(desc, &init_data, &new_render_texture->texture);
	if (!new_render_texture->texture)
	{
		delete new_render_texture;
		return nullptr;
	}

	new_render_texture->srv = new_render_texture->texture->GetDefaultView(Diligent::TEXTURE_VIEW_SHADER_RESOURCE);
	new_render_texture->width = desc.Width;
	new_render_texture->height = desc.Height;

	shared_texture->m_pRenderData = new_render_texture;
	return new_render_texture;
}

Diligent::ITextureView* diligent_get_texture_view(SharedTexture* shared_texture, bool texture_changed)
{
	auto* render_texture = diligent_get_render_texture(shared_texture, texture_changed);
	return render_texture ? render_texture->srv.RawPtr() : nullptr;
}

Diligent::RefCntAutoPtr<Diligent::IBuffer> diligent_create_buffer(
	uint32 size,
	Diligent::BIND_FLAGS bind_flags,
	const void* data,
	uint32 stride)
{
	if (!g_render_device || size == 0)
	{
		return {};
	}

	Diligent::BufferDesc desc;
	desc.Name = "ltjs_buffer";
	desc.Size = size;
	desc.BindFlags = bind_flags;
	desc.Usage = data ? Diligent::USAGE_IMMUTABLE : Diligent::USAGE_DEFAULT;
	desc.CPUAccessFlags = Diligent::CPU_ACCESS_NONE;
	desc.ElementByteStride = stride;

	Diligent::BufferData buffer_data;
	if (data)
	{
		buffer_data.pData = data;
		buffer_data.DataSize = size;
	}

	Diligent::RefCntAutoPtr<Diligent::IBuffer> buffer;
	g_render_device->CreateBuffer(desc, data ? &buffer_data : nullptr, &buffer);
	return buffer;
}

void diligent_UpdateWindowHandles(const RenderStructInit* init)
{
	g_native_window_handle = nullptr;
#ifdef LTJS_SDL_BACKEND
	g_sdl_window = nullptr;
	if (!init)
	{
		return;
	}

	const auto* window_desc = static_cast<const ltjs::MainWindowDescriptor*>(init->m_hWnd);
	if (!window_desc)
	{
		return;
	}

	g_sdl_window = window_desc->sdl_window;
	g_native_window_handle = static_cast<HWND>(window_desc->native_handle);
#else
	if (init)
	{
		g_native_window_handle = static_cast<HWND>(init->m_hWnd);
	}
#endif
}

bool diligent_QueryWindowSize(uint32& width, uint32& height)
{
#ifdef LTJS_SDL_BACKEND
	if (!g_sdl_window)
	{
		return false;
	}

	int window_width = 0;
	int window_height = 0;
	SDL_GetWindowSize(g_sdl_window, &window_width, &window_height);
	if (window_width <= 0 || window_height <= 0)
	{
		return false;
	}

	width = static_cast<uint32>(window_width);
	height = static_cast<uint32>(window_height);
	return true;
#else
	if (!g_native_window_handle)
	{
		return false;
	}

	RECT rect{};
	if (!GetClientRect(g_native_window_handle, &rect))
	{
		return false;
	}

	const int window_width = rect.right - rect.left;
	const int window_height = rect.bottom - rect.top;
	if (window_width <= 0 || window_height <= 0)
	{
		return false;
	}

	width = static_cast<uint32>(window_width);
	height = static_cast<uint32>(window_height);
	return true;
#endif
}

void diligent_UpdateScreenSize(uint32 width, uint32 height)
{
	g_screen_width = width;
	g_screen_height = height;

	if (g_render_struct)
	{
		g_render_struct->m_Width = width;
		g_render_struct->m_Height = height;
	}
}

bool diligent_EnsureDevice()
{
	if (g_render_device && g_immediate_context)
	{
		return true;
	}

	if (!g_engine_factory)
	{
		g_engine_factory = Diligent::LoadAndGetEngineFactoryVk();
	}

	if (!g_engine_factory)
	{
		return false;
	}

	Diligent::EngineVkCreateInfo engine_create_info;
	g_engine_factory->CreateDeviceAndContextsVk(engine_create_info, &g_render_device, &g_immediate_context);
	return g_render_device && g_immediate_context;
}

bool diligent_CreateSwapChain(uint32 width, uint32 height)
{
	if (!g_engine_factory || !g_render_device || !g_immediate_context || !g_native_window_handle)
	{
		return false;
	}

	Diligent::SwapChainDesc swap_chain_desc;
	swap_chain_desc.Width = width;
	swap_chain_desc.Height = height;

	Diligent::Win32NativeWindow window{g_native_window_handle};
	g_engine_factory->CreateSwapChainVk(g_render_device, g_immediate_context, swap_chain_desc, window, &g_swap_chain);
	return static_cast<bool>(g_swap_chain);
}

bool diligent_EnsureSwapChain()
{
	if (!diligent_EnsureDevice())
	{
		return false;
	}

	uint32 width = g_screen_width;
	uint32 height = g_screen_height;
	if (!diligent_QueryWindowSize(width, height))
	{
		if (width == 0 || height == 0)
		{
			return false;
		}
	}

	if (!g_swap_chain)
	{
		if (!diligent_CreateSwapChain(width, height))
		{
			return false;
		}

		diligent_UpdateScreenSize(width, height);
		return true;
	}

	if (width != g_screen_width || height != g_screen_height)
	{
		g_swap_chain->Resize(width, height, Diligent::SURFACE_TRANSFORM_OPTIMAL);
		diligent_UpdateScreenSize(width, height);
	}

	return true;
}

HLTBUFFER diligent_CreateSurface(int width, int height)
{
	if (width <= 0 || height <= 0)
	{
		return LTNULL;
	}

	const auto pixel_count = static_cast<size_t>(width) * static_cast<size_t>(height);
	const auto surface_size = sizeof(DiligentSurface) + (pixel_count - 1) * sizeof(uint32);
	DiligentSurface* surface = nullptr;
	LT_MEM_TRACK_ALLOC(surface = static_cast<DiligentSurface*>(LTMemAlloc(surface_size)), LT_MEM_TYPE_RENDERER);
	if (!surface)
	{
		return LTNULL;
	}

	surface->width = static_cast<uint32>(width);
	surface->height = static_cast<uint32>(height);
	surface->pitch = static_cast<uint32>(width) * sizeof(uint32);
	memset(surface->data, 0, pixel_count * sizeof(uint32));
	return static_cast<HLTBUFFER>(surface);
}

void diligent_DeleteSurface(HLTBUFFER surface_handle)
{
	if (!surface_handle)
	{
		return;
	}

	LTMemFree(surface_handle);
}

void diligent_GetSurfaceInfo(HLTBUFFER surface_handle, uint32* width, uint32* height)
{
	const auto* surface = static_cast<const DiligentSurface*>(surface_handle);
	if (!surface)
	{
		return;
	}

	if (width)
	{
		*width = surface->width;
	}

	if (height)
	{
		*height = surface->height;
	}
}

void* diligent_LockSurface(HLTBUFFER surface_handle, uint32& pitch)
{
	auto* surface = static_cast<DiligentSurface*>(surface_handle);
	if (!surface)
	{
		pitch = 0;
		return nullptr;
	}

	pitch = surface->pitch;
	return surface->data;
}

void diligent_UnlockSurface(HLTBUFFER)
{
}

bool diligent_OptimizeSurface(HLTBUFFER, uint32)
{
	return false;
}

void diligent_UnoptimizeSurface(HLTBUFFER)
{
}

bool diligent_GetScreenFormat(PFormat* format)
{
	if (!format)
	{
		return false;
	}

	format->Init(BPP_32, 0xFF000000u, 0x00FF0000u, 0x0000FF00u, 0x000000FFu);
	return true;
}

void diligent_Clear(LTRect*, uint32, LTRGBColor& clear_color)
{
	if (!diligent_EnsureSwapChain())
	{
		return;
	}

	const float clear_rgba[4] = {
		static_cast<float>(clear_color.rgb.r) / 255.0f,
		static_cast<float>(clear_color.rgb.g) / 255.0f,
		static_cast<float>(clear_color.rgb.b) / 255.0f,
		static_cast<float>(clear_color.rgb.a) / 255.0f};

	auto* render_target = g_swap_chain->GetCurrentBackBufferRTV();
	auto* depth_target = g_swap_chain->GetDepthBufferDSV();

	if (!render_target)
	{
		return;
	}

	g_immediate_context->ClearRenderTarget(
		render_target,
		clear_rgba,
		Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

	if (depth_target)
	{
		g_immediate_context->ClearDepthStencil(
			depth_target,
			Diligent::CLEAR_DEPTH_FLAG,
			1.0f,
			0,
			Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
	}
}

bool diligent_Start3D()
{
	g_is_in_3d = true;
	if (g_render_struct)
	{
		++g_render_struct->m_nIn3D;
	}
	return true;
}

bool diligent_End3D()
{
	g_is_in_3d = false;
	if (g_render_struct && g_render_struct->m_nIn3D > 0)
	{
		--g_render_struct->m_nIn3D;
	}
	return true;
}

bool diligent_IsIn3D()
{
	return g_is_in_3d;
}

bool diligent_StartOptimized2D()
{
	g_is_in_optimized_2d = true;
	if (g_render_struct)
	{
		++g_render_struct->m_nInOptimized2D;
	}
	return true;
}

void diligent_EndOptimized2D()
{
	g_is_in_optimized_2d = false;
	if (g_render_struct && g_render_struct->m_nInOptimized2D > 0)
	{
		--g_render_struct->m_nInOptimized2D;
	}
}

bool diligent_IsInOptimized2D()
{
	return g_is_in_optimized_2d;
}

bool diligent_SetOptimized2DBlend(LTSurfaceBlend)
{
	return false;
}

bool diligent_GetOptimized2DBlend(LTSurfaceBlend&)
{
	return false;
}

bool diligent_SetOptimized2DColor(HLTCOLOR)
{
	return false;
}

bool diligent_GetOptimized2DColor(HLTCOLOR&)
{
	return false;
}

int diligent_RenderScene(SceneDesc*)
{
	return RENDER_OK;
}

void diligent_RenderCommand(int, const char**)
{
}

void diligent_SwapBuffers(uint32)
{
	if (!diligent_EnsureSwapChain())
	{
		return;
	}

	g_swap_chain->Present(1);
}

bool diligent_LockScreen(int, int, int, int, void**, long*)
{
	return false;
}

void diligent_UnlockScreen()
{
}

void diligent_BlitToScreen(BlitRequest*)
{
}

void diligent_BlitFromScreen(BlitRequest*)
{
}

bool diligent_WarpToScreen(BlitRequest*)
{
	return false;
}

void diligent_MakeScreenShot(const char*)
{
}

void diligent_MakeCubicEnvMap(const char*, uint32, const SceneDesc&)
{
}

void diligent_ReadConsoleVariables()
{
}

void diligent_GetRenderInfo(RenderInfoStruct*)
{
}

HRENDERCONTEXT diligent_CreateContext()
{
	void* context = nullptr;
	LT_MEM_TRACK_ALLOC(context = LTMemAlloc(1), LT_MEM_TYPE_RENDERER);
	return static_cast<HRENDERCONTEXT>(context);
}

void diligent_DeleteContext(HRENDERCONTEXT context)
{
	if (context)
	{
		LTMemFree(context);
	}
}

void diligent_BindTexture(SharedTexture* texture, bool texture_changed)
{
	diligent_get_texture_view(texture, texture_changed);
}

void diligent_UnbindTexture(SharedTexture* texture)
{
	diligent_release_render_texture(texture);
}

uint32 diligent_GetTextureFormat1(BPPIdent, uint32)
{
	return 0;
}

bool diligent_QueryDDSupport(PFormat*)
{
	return false;
}

bool diligent_GetTextureFormat2(BPPIdent, uint32, PFormat*)
{
	return false;
}

bool diligent_ConvertTexDataToDD(uint8*, PFormat*, uint32, uint32, uint8*, PFormat*, BPPIdent, uint32, uint32, uint32)
{
	return false;
}

void diligent_DrawPrimSetTexture(SharedTexture* texture)
{
	g_drawprim_texture = texture;
	diligent_get_texture_view(texture, false);
}

void diligent_DrawPrimDisableTextures()
{
}

CRenderObject* diligent_CreateRenderObject(CRenderObject::RENDER_OBJECT_TYPES)
{
	return nullptr;
}

bool diligent_DestroyRenderObject(CRenderObject*)
{
	return false;
}

bool diligent_LoadWorldData(ILTStream*)
{
	return false;
}

bool diligent_SetLightGroupColor(uint32, const LTVector&)
{
	return false;
}

LTRESULT diligent_SetOccluderEnabled(uint32, bool)
{
	return LT_NOTFOUND;
}

LTRESULT diligent_GetOccluderEnabled(uint32, bool*)
{
	return LT_NOTFOUND;
}

uint32 diligent_GetTextureEffectVarID(const char*, uint32)
{
	return 0;
}

bool diligent_SetTextureEffectVar(uint32, uint32, float)
{
	return false;
}

bool diligent_IsObjectGroupEnabled(uint32)
{
	return false;
}

void diligent_SetObjectGroupEnabled(uint32, bool)
{
}

void diligent_SetAllObjectGroupEnabled()
{
}

bool diligent_AddGlowRenderStyleMapping(const char*, const char*)
{
	return false;
}

bool diligent_SetGlowDefaultRenderStyle(const char*)
{
	return false;
}

bool diligent_SetNoGlowRenderStyle(const char*)
{
	return false;
}

int diligent_Init(RenderStructInit* init)
{
	if (!init)
	{
		return RENDER_ERROR;
	}

	init->m_RendererVersion = LTRENDER_VERSION;
	g_screen_width = init->m_Mode.m_Width;
	g_screen_height = init->m_Mode.m_Height;

	diligent_UpdateWindowHandles(init);
	if (!g_native_window_handle)
	{
		return RENDER_ERROR;
	}

	if (!diligent_EnsureDevice())
	{
		return RENDER_ERROR;
	}

	diligent_UpdateScreenSize(g_screen_width, g_screen_height);
	if (!diligent_EnsureSwapChain())
	{
		return RENDER_ERROR;
	}

	return RENDER_OK;
}

void diligent_Term(bool)
{
	g_swap_chain.Release();
	g_immediate_context.Release();
	g_render_device.Release();
	g_pso_cache.Reset();
	g_srb_cache.Reset();
	g_engine_factory = nullptr;
	g_drawprim_texture = nullptr;
	g_native_window_handle = nullptr;
#ifdef LTJS_SDL_BACKEND
	g_sdl_window = nullptr;
#endif
	g_screen_width = 0;
	g_screen_height = 0;
	g_is_in_3d = false;
	g_is_in_optimized_2d = false;
}

#if !defined(LTJS_USE_DILIGENT_RENDER)
void* diligent_GetD3DDevice()
{
	return nullptr;
}
#endif

Diligent::IRenderDevice* diligent_GetRenderDevice()
{
	return g_render_device.RawPtr();
}

Diligent::IDeviceContext* diligent_GetImmediateContext()
{
	return g_immediate_context.RawPtr();
}

Diligent::ISwapChain* diligent_GetSwapChain()
{
	return g_swap_chain.RawPtr();
}

RMode* rdll_GetSupportedModes()
{
	RMode* mode = nullptr;
	LT_MEM_TRACK_ALLOC(mode = new RMode, LT_MEM_TYPE_RENDERER);
	if (!mode)
	{
		return nullptr;
	}

	LTStrCpy(mode->m_Description, "Diligent (Vulkan)", sizeof(mode->m_Description));
	LTStrCpy(mode->m_InternalName, "DiligentVulkan", sizeof(mode->m_InternalName));
	mode->m_Width = 1280;
	mode->m_Height = 720;
	mode->m_BitDepth = 32;
	mode->m_bHWTnL = true;
	mode->m_pNext = LTNULL;
	return mode;
}

void rdll_FreeModeList(RMode* modes)
{
	auto* current = modes;
	while (current)
	{
		auto* next = current->m_pNext;
		LTMemFree(current);
		current = next;
	}
}
}

void rdll_RenderDLLSetup(RenderStruct* render_struct)
{
	g_render_struct = render_struct;

	render_struct->Init = diligent_Init;
	render_struct->Term = diligent_Term;
#if !defined(LTJS_USE_DILIGENT_RENDER)
	render_struct->GetD3DDevice = diligent_GetD3DDevice;
#endif
	render_struct->GetRenderDevice = diligent_GetRenderDevice;
	render_struct->GetImmediateContext = diligent_GetImmediateContext;
	render_struct->GetSwapChain = diligent_GetSwapChain;
	render_struct->BindTexture = diligent_BindTexture;
	render_struct->UnbindTexture = diligent_UnbindTexture;
	render_struct->GetTextureDDFormat1 = diligent_GetTextureFormat1;
	render_struct->QueryDDSupport = diligent_QueryDDSupport;
	render_struct->GetTextureDDFormat2 = diligent_GetTextureFormat2;
	render_struct->ConvertTexDataToDD = diligent_ConvertTexDataToDD;
	render_struct->DrawPrimSetTexture = diligent_DrawPrimSetTexture;
	render_struct->DrawPrimDisableTextures = diligent_DrawPrimDisableTextures;
	render_struct->CreateContext = diligent_CreateContext;
	render_struct->DeleteContext = diligent_DeleteContext;
	render_struct->Clear = diligent_Clear;
	render_struct->Start3D = diligent_Start3D;
	render_struct->End3D = diligent_End3D;
	render_struct->IsIn3D = diligent_IsIn3D;
	render_struct->StartOptimized2D = diligent_StartOptimized2D;
	render_struct->EndOptimized2D = diligent_EndOptimized2D;
	render_struct->IsInOptimized2D = diligent_IsInOptimized2D;
	render_struct->SetOptimized2DBlend = diligent_SetOptimized2DBlend;
	render_struct->GetOptimized2DBlend = diligent_GetOptimized2DBlend;
	render_struct->SetOptimized2DColor = diligent_SetOptimized2DColor;
	render_struct->GetOptimized2DColor = diligent_GetOptimized2DColor;
	render_struct->RenderScene = diligent_RenderScene;
	render_struct->RenderCommand = diligent_RenderCommand;
	render_struct->SwapBuffers = diligent_SwapBuffers;
	render_struct->GetScreenFormat = diligent_GetScreenFormat;
	render_struct->CreateSurface = diligent_CreateSurface;
	render_struct->DeleteSurface = diligent_DeleteSurface;
	render_struct->GetSurfaceInfo = diligent_GetSurfaceInfo;
	render_struct->LockSurface = diligent_LockSurface;
	render_struct->UnlockSurface = diligent_UnlockSurface;
	render_struct->OptimizeSurface = diligent_OptimizeSurface;
	render_struct->UnoptimizeSurface = diligent_UnoptimizeSurface;
	render_struct->LockScreen = diligent_LockScreen;
	render_struct->UnlockScreen = diligent_UnlockScreen;
	render_struct->BlitToScreen = diligent_BlitToScreen;
	render_struct->BlitFromScreen = diligent_BlitFromScreen;
	render_struct->WarpToScreen = diligent_WarpToScreen;
	render_struct->MakeScreenShot = diligent_MakeScreenShot;
	render_struct->MakeCubicEnvMap = diligent_MakeCubicEnvMap;
	render_struct->ReadConsoleVariables = diligent_ReadConsoleVariables;
	render_struct->GetRenderInfo = diligent_GetRenderInfo;
	render_struct->CreateRenderObject = diligent_CreateRenderObject;
	render_struct->DestroyRenderObject = diligent_DestroyRenderObject;
	render_struct->LoadWorldData = diligent_LoadWorldData;
	render_struct->SetLightGroupColor = diligent_SetLightGroupColor;
	render_struct->SetOccluderEnabled = diligent_SetOccluderEnabled;
	render_struct->GetOccluderEnabled = diligent_GetOccluderEnabled;
	render_struct->GetTextureEffectVarID = diligent_GetTextureEffectVarID;
	render_struct->SetTextureEffectVar = diligent_SetTextureEffectVar;
	render_struct->IsObjectGroupEnabled = diligent_IsObjectGroupEnabled;
	render_struct->SetObjectGroupEnabled = diligent_SetObjectGroupEnabled;
	render_struct->SetAllObjectGroupEnabled = diligent_SetAllObjectGroupEnabled;
	render_struct->AddGlowRenderStyleMapping = diligent_AddGlowRenderStyleMapping;
	render_struct->SetGlowDefaultRenderStyle = diligent_SetGlowDefaultRenderStyle;
	render_struct->SetNoGlowRenderStyle = diligent_SetNoGlowRenderStyle;
}
