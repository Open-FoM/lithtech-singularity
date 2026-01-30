#pragma once

/// Solid brush renderer for DEdit2 viewports.
///
/// Renders editor-created brushes as solid filled geometry with textures.
/// Supports per-face texture assignment with planar UV projection.

#include "DiligentCore/Common/interface/BasicMath.hpp"
#include "DiligentCore/Common/interface/RefCntAutoPtr.hpp"
#include "DiligentCore/Graphics/GraphicsEngine/interface/Buffer.h"
#include "DiligentCore/Graphics/GraphicsEngine/interface/DeviceContext.h"
#include "DiligentCore/Graphics/GraphicsEngine/interface/GraphicsTypes.h"
#include "DiligentCore/Graphics/GraphicsEngine/interface/PipelineState.h"
#include "DiligentCore/Graphics/GraphicsEngine/interface/RenderDevice.h"
#include "DiligentCore/Graphics/GraphicsEngine/interface/Sampler.h"
#include "DiligentCore/Graphics/GraphicsEngine/interface/Texture.h"

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

struct NodeProperties;
struct TreeNode;
class TextureCache;

/// GPU texture for brush rendering.
/// Note: srv is borrowed from SharedTexture, not owned.
struct BrushGPUTexture
{
  Diligent::ITextureView* srv = nullptr;  ///< Borrowed from engine's texture cache
  uint32_t width = 0;
  uint32_t height = 0;
};

/// GPU data for a single brush face batch (faces sharing same texture).
struct BrushFaceBatch
{
  Diligent::RefCntAutoPtr<Diligent::IBuffer> vertex_buffer;
  Diligent::RefCntAutoPtr<Diligent::IShaderResourceBinding> srb;
  uint32_t vertex_count = 0;
  std::string texture_name;
  bool has_texture = false;
};

/// GPU data for a single brush.
struct BrushRenderData
{
  std::vector<BrushFaceBatch> batches;
  int node_id = -1;
};

/// Renderer for solid brush geometry with textures.
struct BrushRenderer
{
  Diligent::RefCntAutoPtr<Diligent::IPipelineState> pipeline_textured;
  Diligent::RefCntAutoPtr<Diligent::IPipelineState> pipeline_untextured;
  Diligent::RefCntAutoPtr<Diligent::IShaderResourceBinding> srb_untextured;
  Diligent::RefCntAutoPtr<Diligent::IBuffer> constant_buffer;
  Diligent::RefCntAutoPtr<Diligent::ISampler> sampler;
  Diligent::IRenderDevice* device = nullptr;

  std::vector<BrushRenderData> brushes;
  std::unordered_map<std::string, BrushGPUTexture> gpu_textures;

  /// Default white texture for untextured faces (owned by renderer)
  Diligent::RefCntAutoPtr<Diligent::ITexture> default_texture;
  Diligent::ITextureView* default_texture_srv = nullptr;

  bool ready = false;
};

/// Initialize the brush renderer with graphics resources.
/// @param device Render device for creating GPU resources.
/// @param rtv_format Render target format.
/// @param dsv_format Depth stencil format.
/// @param renderer Output renderer to initialize.
/// @return true if initialization succeeded.
bool InitBrushRenderer(
    Diligent::IRenderDevice* device,
    Diligent::TEXTURE_FORMAT rtv_format,
    Diligent::TEXTURE_FORMAT dsv_format,
    BrushRenderer& renderer);

/// Update brush geometry buffers from scene data.
/// Call this when brushes are created, modified, or deleted.
/// @param device Render device for creating GPU buffers.
/// @param renderer Brush renderer to update.
/// @param nodes Scene nodes to scan for brushes.
/// @param props Node properties containing brush geometry.
/// @param texture_cache Texture cache for loading textures.
/// @param project_root Project root path for texture resolution.
void UpdateBrushGeometry(
    Diligent::IRenderDevice* device,
    BrushRenderer& renderer,
    const std::vector<TreeNode>& nodes,
    const std::vector<NodeProperties>& props,
    TextureCache* texture_cache = nullptr,
    const std::string& project_root = "");

/// Draw all brush geometry.
/// @param context Device context for rendering.
/// @param renderer Brush renderer to draw with.
/// @param view_proj View-projection matrix.
void DrawBrushes(
    Diligent::IDeviceContext* context,
    BrushRenderer& renderer,
    const Diligent::float4x4& view_proj);
