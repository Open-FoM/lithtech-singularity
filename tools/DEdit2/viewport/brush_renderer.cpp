#include "viewport/brush_renderer.h"

#include "diligent_texture_cache.h"
#include "editor_state.h"
#include "texture_cache.h"

#include "DiligentCore/Graphics/GraphicsTools/interface/MapHelper.hpp"

#include <cmath>
#include <utility>

namespace
{

struct BrushVertex
{
  Diligent::float3 position;
  Diligent::float3 normal;
  Diligent::float2 uv;
  Diligent::float4 color;
};

struct BrushConstants
{
  Diligent::float4x4 view_proj;
};

Diligent::float3 ComputeTriangleNormal(
    const Diligent::float3& v0,
    const Diligent::float3& v1,
    const Diligent::float3& v2)
{
  Diligent::float3 edge1{v1.x - v0.x, v1.y - v0.y, v1.z - v0.z};
  Diligent::float3 edge2{v2.x - v0.x, v2.y - v0.y, v2.z - v0.z};

  // Cross product
  Diligent::float3 normal{
      edge1.y * edge2.z - edge1.z * edge2.y,
      edge1.z * edge2.x - edge1.x * edge2.z,
      edge1.x * edge2.y - edge1.y * edge2.x};

  // Normalize
  float len = std::sqrt(normal.x * normal.x + normal.y * normal.y + normal.z * normal.z);
  if (len > 0.0001f)
  {
    normal.x /= len;
    normal.y /= len;
    normal.z /= len;
  }
  return normal;
}

/// Compute planar UV coordinates for a vertex based on face normal.
/// Uses world-space projection with configurable texture scale.
Diligent::float2 ComputePlanarUV(
    const Diligent::float3& pos,
    const Diligent::float3& normal,
    const texture_ops::TextureMapping& mapping,
    float tex_width,
    float tex_height)
{
  // Determine dominant axis for projection plane
  float abs_x = std::abs(normal.x);
  float abs_y = std::abs(normal.y);
  float abs_z = std::abs(normal.z);

  float u_world, v_world;

  if (abs_y >= abs_x && abs_y >= abs_z)
  {
    // Y-dominant: project onto XZ plane (floor/ceiling)
    u_world = pos.x;
    v_world = pos.z;
  }
  else if (abs_x >= abs_z)
  {
    // X-dominant: project onto YZ plane (left/right walls)
    u_world = pos.z;
    v_world = pos.y;
  }
  else
  {
    // Z-dominant: project onto XY plane (front/back walls)
    u_world = pos.x;
    v_world = pos.y;
  }

  // Convert world units to UV space
  // Default: 1 texture unit = 64 world units (standard Quake/LithTech scale)
  constexpr float kWorldToTexScale = 1.0f / 64.0f;
  float u = u_world * kWorldToTexScale;
  float v = v_world * kWorldToTexScale;

  // Apply texture mapping transforms
  // Scale
  u *= mapping.scale_u;
  v *= mapping.scale_v;

  // Rotate (convert degrees to radians)
  if (std::abs(mapping.rotation) > 0.001f)
  {
    float rad = mapping.rotation * (3.14159265358979323846f / 180.0f);
    float cos_r = std::cos(rad);
    float sin_r = std::sin(rad);
    float rotated_u = u * cos_r - v * sin_r;
    float rotated_v = u * sin_r + v * cos_r;
    u = rotated_u;
    v = rotated_v;
  }

  // Offset (normalized to texture dimensions)
  u += mapping.offset_u / tex_width;
  v += mapping.offset_v / tex_height;

  return Diligent::float2{u, v};
}

/// Create a 1x1 white texture for untextured faces.
bool CreateDefaultTexture(Diligent::IRenderDevice* device, BrushRenderer& renderer)
{
  Diligent::TextureDesc tex_desc;
  tex_desc.Name = "DEdit2 Brush Default Texture";
  tex_desc.Type = Diligent::RESOURCE_DIM_TEX_2D;
  tex_desc.Width = 1;
  tex_desc.Height = 1;
  tex_desc.Format = Diligent::TEX_FORMAT_RGBA8_UNORM;
  tex_desc.MipLevels = 1;
  tex_desc.Usage = Diligent::USAGE_IMMUTABLE;
  tex_desc.BindFlags = Diligent::BIND_SHADER_RESOURCE;

  // White pixel (RGBA)
  uint32_t white_pixel = 0xFFFFFFFF;
  Diligent::TextureSubResData sub_res;
  sub_res.pData = &white_pixel;
  sub_res.Stride = 4;

  Diligent::TextureData tex_data;
  tex_data.pSubResources = &sub_res;
  tex_data.NumSubresources = 1;

  device->CreateTexture(tex_desc, &tex_data, &renderer.default_texture);
  if (!renderer.default_texture)
  {
    return false;
  }

  renderer.default_texture_srv = renderer.default_texture->GetDefaultView(Diligent::TEXTURE_VIEW_SHADER_RESOURCE);
  return renderer.default_texture_srv != nullptr;
}

/// Load a texture using the engine's texture cache (handles format conversion).
/// Returns the texture view, or nullptr if loading failed.
Diligent::ITextureView* LoadTextureView(
    TextureCache* cache,
    const std::string& texture_name,
    uint32_t& out_width,
    uint32_t& out_height)
{
  if (cache == nullptr || texture_name.empty())
  {
    return nullptr;
  }

  SharedTexture* shared = cache->GetSharedTexture(texture_name.c_str());
  if (shared == nullptr)
  {
    return nullptr;
  }

  // Use the engine's texture view getter - handles all format conversion
  Diligent::ITextureView* srv = diligent_get_texture_view(shared, false);
  if (srv != nullptr)
  {
    // Get texture dimensions from TextureInfo
    uint32_t w = 0, h = 0;
    PFormat fmt;
    shared->GetTextureInfo(w, h, fmt);
    out_width = w;
    out_height = h;
  }
  return srv;
}

} // namespace

bool InitBrushRenderer(
    Diligent::IRenderDevice* device,
    Diligent::TEXTURE_FORMAT rtv_format,
    Diligent::TEXTURE_FORMAT dsv_format,
    BrushRenderer& renderer)
{
  if (device == nullptr)
  {
    return false;
  }

  renderer.device = device;

  // Create constant buffer
  Diligent::BufferDesc cb_desc;
  cb_desc.Name = "DEdit2 Brush CB";
  cb_desc.Usage = Diligent::USAGE_DYNAMIC;
  cb_desc.BindFlags = Diligent::BIND_UNIFORM_BUFFER;
  cb_desc.CPUAccessFlags = Diligent::CPU_ACCESS_WRITE;
  cb_desc.Size = sizeof(BrushConstants);
  device->CreateBuffer(cb_desc, nullptr, &renderer.constant_buffer);
  if (!renderer.constant_buffer)
  {
    return false;
  }

  // Create sampler for textures
  Diligent::SamplerDesc sampler_desc;
  sampler_desc.Name = "DEdit2 Brush Sampler";
  sampler_desc.MinFilter = Diligent::FILTER_TYPE_LINEAR;
  sampler_desc.MagFilter = Diligent::FILTER_TYPE_LINEAR;
  sampler_desc.MipFilter = Diligent::FILTER_TYPE_LINEAR;
  sampler_desc.AddressU = Diligent::TEXTURE_ADDRESS_WRAP;
  sampler_desc.AddressV = Diligent::TEXTURE_ADDRESS_WRAP;
  sampler_desc.AddressW = Diligent::TEXTURE_ADDRESS_WRAP;
  device->CreateSampler(sampler_desc, &renderer.sampler);
  if (!renderer.sampler)
  {
    return false;
  }

  // Create default white texture
  if (!CreateDefaultTexture(device, renderer))
  {
    return false;
  }

  // Textured vertex shader with directional lighting
  static const char* vs_textured_source = R"(
cbuffer BrushData : register(b0)
{
    float4x4 g_ViewProj;
};

struct VSInput
{
    float3 Pos    : ATTRIB0;
    float3 Normal : ATTRIB1;
    float2 UV     : ATTRIB2;
    float4 Color  : ATTRIB3;
};

struct PSInput
{
    float4 Pos   : SV_POSITION;
    float2 UV    : TEXCOORD0;
    float4 Color : COLOR0;
};

PSInput main(VSInput input)
{
    PSInput output;
    output.Pos = mul(g_ViewProj, float4(input.Pos, 1.0f));
    output.UV = input.UV;

    // Simple directional lighting
    float3 lightDir = normalize(float3(0.5f, 1.0f, 0.3f));
    float ndl = max(0.3f, dot(input.Normal, lightDir));
    output.Color = float4(input.Color.rgb * ndl, input.Color.a);
    return output;
}
)";

  // Textured pixel shader
  static const char* ps_textured_source = R"(
Texture2D    g_Texture : register(t0);
SamplerState g_Sampler : register(s0);

struct PSInput
{
    float4 Pos   : SV_POSITION;
    float2 UV    : TEXCOORD0;
    float4 Color : COLOR0;
};

float4 main(PSInput input) : SV_TARGET
{
    float4 texColor = g_Texture.Sample(g_Sampler, input.UV);
    return texColor * input.Color;
}
)";

  // Untextured pixel shader (uses vertex color only)
  static const char* ps_untextured_source = R"(
struct PSInput
{
    float4 Pos   : SV_POSITION;
    float2 UV    : TEXCOORD0;
    float4 Color : COLOR0;
};

float4 main(PSInput input) : SV_TARGET
{
    return input.Color;
}
)";

  Diligent::ShaderCreateInfo shader_ci;
  shader_ci.SourceLanguage = Diligent::SHADER_SOURCE_LANGUAGE_HLSL;

  // Compile vertex shader (shared between textured and untextured)
  Diligent::RefCntAutoPtr<Diligent::IShader> vs;
  shader_ci.Desc.ShaderType = Diligent::SHADER_TYPE_VERTEX;
  shader_ci.Desc.Name = "DEdit2 Brush VS";
  shader_ci.EntryPoint = "main";
  shader_ci.Source = vs_textured_source;
  device->CreateShader(shader_ci, &vs);
  if (!vs)
  {
    return false;
  }

  // Compile textured pixel shader
  Diligent::RefCntAutoPtr<Diligent::IShader> ps_textured;
  shader_ci.Desc.ShaderType = Diligent::SHADER_TYPE_PIXEL;
  shader_ci.Desc.Name = "DEdit2 Brush PS Textured";
  shader_ci.EntryPoint = "main";
  shader_ci.Source = ps_textured_source;
  device->CreateShader(shader_ci, &ps_textured);
  if (!ps_textured)
  {
    return false;
  }

  // Compile untextured pixel shader
  Diligent::RefCntAutoPtr<Diligent::IShader> ps_untextured;
  shader_ci.Desc.ShaderType = Diligent::SHADER_TYPE_PIXEL;
  shader_ci.Desc.Name = "DEdit2 Brush PS Untextured";
  shader_ci.EntryPoint = "main";
  shader_ci.Source = ps_untextured_source;
  device->CreateShader(shader_ci, &ps_untextured);
  if (!ps_untextured)
  {
    return false;
  }

  // Vertex layout: position (float3), normal (float3), uv (float2), color (float4)
  Diligent::LayoutElement layout[] = {
      Diligent::LayoutElement{0, 0, 3, Diligent::VT_FLOAT32, false}, // Position
      Diligent::LayoutElement{1, 0, 3, Diligent::VT_FLOAT32, false}, // Normal
      Diligent::LayoutElement{2, 0, 2, Diligent::VT_FLOAT32, false}, // UV
      Diligent::LayoutElement{3, 0, 4, Diligent::VT_FLOAT32, false}  // Color
  };

  // Create textured pipeline state
  {
    Diligent::GraphicsPipelineStateCreateInfo pso_ci;
    pso_ci.PSODesc.Name = "DEdit2 Brush PSO Textured";
    pso_ci.pVS = vs;
    pso_ci.pPS = ps_textured;

    // Resource layout for textured shader
    Diligent::ShaderResourceVariableDesc vars[] = {
        {Diligent::SHADER_TYPE_PIXEL, "g_Texture", Diligent::SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC}};
    pso_ci.PSODesc.ResourceLayout.Variables = vars;
    pso_ci.PSODesc.ResourceLayout.NumVariables = 1;

    Diligent::ImmutableSamplerDesc samplers[] = {
        {Diligent::SHADER_TYPE_PIXEL, "g_Sampler", sampler_desc}};
    pso_ci.PSODesc.ResourceLayout.ImmutableSamplers = samplers;
    pso_ci.PSODesc.ResourceLayout.NumImmutableSamplers = 1;

    auto& gp = pso_ci.GraphicsPipeline;
    gp.PrimitiveTopology = Diligent::PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    gp.NumRenderTargets = 1;
    gp.RTVFormats[0] = rtv_format;
    gp.DSVFormat = dsv_format;
    gp.RasterizerDesc.CullMode = Diligent::CULL_MODE_BACK;
    gp.DepthStencilDesc.DepthEnable = true;
    gp.DepthStencilDesc.DepthWriteEnable = true;
    gp.DepthStencilDesc.DepthFunc = Diligent::COMPARISON_FUNC_LESS;
    gp.BlendDesc.RenderTargets[0].BlendEnable = false;

    gp.InputLayout.LayoutElements = layout;
    gp.InputLayout.NumElements = static_cast<Diligent::Uint32>(sizeof(layout) / sizeof(layout[0]));

    device->CreateGraphicsPipelineState(pso_ci, &renderer.pipeline_textured);
    if (!renderer.pipeline_textured)
    {
      return false;
    }

    if (auto* var = renderer.pipeline_textured->GetStaticVariableByName(Diligent::SHADER_TYPE_VERTEX, "BrushData"))
    {
      var->Set(renderer.constant_buffer);
    }
  }

  // Create untextured pipeline state
  {
    Diligent::GraphicsPipelineStateCreateInfo pso_ci;
    pso_ci.PSODesc.Name = "DEdit2 Brush PSO Untextured";
    pso_ci.PSODesc.ResourceLayout.DefaultVariableType = Diligent::SHADER_RESOURCE_VARIABLE_TYPE_STATIC;
    pso_ci.pVS = vs;
    pso_ci.pPS = ps_untextured;

    auto& gp = pso_ci.GraphicsPipeline;
    gp.PrimitiveTopology = Diligent::PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    gp.NumRenderTargets = 1;
    gp.RTVFormats[0] = rtv_format;
    gp.DSVFormat = dsv_format;
    gp.RasterizerDesc.CullMode = Diligent::CULL_MODE_BACK;
    gp.DepthStencilDesc.DepthEnable = true;
    gp.DepthStencilDesc.DepthWriteEnable = true;
    gp.DepthStencilDesc.DepthFunc = Diligent::COMPARISON_FUNC_LESS;
    gp.BlendDesc.RenderTargets[0].BlendEnable = false;

    gp.InputLayout.LayoutElements = layout;
    gp.InputLayout.NumElements = static_cast<Diligent::Uint32>(sizeof(layout) / sizeof(layout[0]));

    device->CreateGraphicsPipelineState(pso_ci, &renderer.pipeline_untextured);
    if (!renderer.pipeline_untextured)
    {
      return false;
    }

    if (auto* var = renderer.pipeline_untextured->GetStaticVariableByName(Diligent::SHADER_TYPE_VERTEX, "BrushData"))
    {
      var->Set(renderer.constant_buffer);
    }
    renderer.pipeline_untextured->CreateShaderResourceBinding(&renderer.srb_untextured, true);
  }

  renderer.ready = renderer.pipeline_textured != nullptr && renderer.pipeline_untextured != nullptr;
  return renderer.ready;
}

void UpdateBrushGeometry(
    Diligent::IRenderDevice* device,
    BrushRenderer& renderer,
    const std::vector<TreeNode>& nodes,
    const std::vector<NodeProperties>& props,
    TextureCache* texture_cache,
    const std::string& project_root)
{
  (void)project_root; // Used for texture path resolution via TextureCache

  renderer.brushes.clear();

  if (device == nullptr || nodes.empty())
  {
    return;
  }

  for (size_t i = 0; i < nodes.size(); ++i)
  {
    const TreeNode& node = nodes[i];
    if (node.deleted || node.is_folder)
    {
      continue;
    }

    if (i >= props.size())
    {
      continue;
    }

    const NodeProperties& prop = props[i];
    if (prop.type != "Brush")
    {
      continue;
    }

    if (prop.brush_vertices.empty() || prop.brush_indices.empty())
    {
      continue;
    }

    const auto& verts = prop.brush_vertices;
    const auto& indices = prop.brush_indices;
    const auto& face_textures = prop.brush_face_textures;

    // Group triangles by texture for batching
    std::unordered_map<std::string, std::vector<BrushVertex>> texture_batches;

    // Default white color for all faces
    const Diligent::float4 base_color{1.0f, 1.0f, 1.0f, 1.0f};

    // Process each triangle
    size_t face_index = 0;
    for (size_t t = 0; t + 2 < indices.size(); t += 3, ++face_index)
    {
      uint32_t i0 = indices[t];
      uint32_t i1 = indices[t + 1];
      uint32_t i2 = indices[t + 2];

      // Reverse winding order: brush primitives have inward-facing winding,
      // swap i1 and i2 to get outward-facing normals for proper culling
      std::swap(i1, i2);

      // Validate indices
      if (static_cast<size_t>(i0) * 3 + 2 >= verts.size() ||
          static_cast<size_t>(i1) * 3 + 2 >= verts.size() ||
          static_cast<size_t>(i2) * 3 + 2 >= verts.size())
      {
        continue;
      }

      Diligent::float3 v0{verts[i0 * 3], verts[i0 * 3 + 1], verts[i0 * 3 + 2]};
      Diligent::float3 v1{verts[i1 * 3], verts[i1 * 3 + 1], verts[i1 * 3 + 2]};
      Diligent::float3 v2{verts[i2 * 3], verts[i2 * 3 + 1], verts[i2 * 3 + 2]};

      Diligent::float3 normal = ComputeTriangleNormal(v0, v1, v2);

      // Get texture info for this face
      std::string tex_name;
      texture_ops::TextureMapping mapping;
      float tex_width = 64.0f;
      float tex_height = 64.0f;

      if (face_index < face_textures.size())
      {
        const auto& tex = face_textures[face_index];
        if (!tex.texture_name.empty() && tex.texture_name != "default")
        {
          tex_name = tex.texture_name;
          mapping = tex.mapping;

          // Try to get actual texture dimensions
          auto gpu_it = renderer.gpu_textures.find(tex_name);
          if (gpu_it != renderer.gpu_textures.end())
          {
            tex_width = static_cast<float>(gpu_it->second.width);
            tex_height = static_cast<float>(gpu_it->second.height);
          }
          else if (texture_cache)
          {
            // Load texture if not already loaded (uses engine's format conversion)
            uint32_t w = 0, h = 0;
            Diligent::ITextureView* srv = LoadTextureView(texture_cache, tex_name, w, h);
            if (srv != nullptr)
            {
              BrushGPUTexture gpu_tex;
              gpu_tex.srv = srv;
              gpu_tex.width = w;
              gpu_tex.height = h;
              tex_width = static_cast<float>(w);
              tex_height = static_cast<float>(h);
              renderer.gpu_textures[tex_name] = gpu_tex;
            }
          }
        }
      }

      // Compute UV coordinates using planar projection
      Diligent::float2 uv0 = ComputePlanarUV(v0, normal, mapping, tex_width, tex_height);
      Diligent::float2 uv1 = ComputePlanarUV(v1, normal, mapping, tex_width, tex_height);
      Diligent::float2 uv2 = ComputePlanarUV(v2, normal, mapping, tex_width, tex_height);

      // Add vertices to appropriate batch
      std::vector<BrushVertex>& batch = texture_batches[tex_name];
      batch.push_back({v0, normal, uv0, base_color});
      batch.push_back({v1, normal, uv1, base_color});
      batch.push_back({v2, normal, uv2, base_color});
    }

    // Create GPU buffers for each texture batch
    BrushRenderData data;
    data.node_id = static_cast<int>(i);

    for (auto& [tex_name, vertices] : texture_batches)
    {
      if (vertices.empty())
      {
        continue;
      }

      BrushFaceBatch batch;
      batch.texture_name = tex_name;
      batch.has_texture = !tex_name.empty();
      batch.vertex_count = static_cast<uint32_t>(vertices.size());

      Diligent::BufferDesc vb_desc;
      vb_desc.Name = "DEdit2 Brush VB";
      vb_desc.Usage = Diligent::USAGE_IMMUTABLE;
      vb_desc.BindFlags = Diligent::BIND_VERTEX_BUFFER;
      vb_desc.Size = static_cast<Diligent::Uint64>(vertices.size() * sizeof(BrushVertex));

      Diligent::BufferData vb_data;
      vb_data.pData = vertices.data();
      vb_data.DataSize = vb_desc.Size;
      device->CreateBuffer(vb_desc, &vb_data, &batch.vertex_buffer);

      if (!batch.vertex_buffer)
      {
        continue;
      }

      // Create SRB for textured batches
      if (batch.has_texture && renderer.pipeline_textured)
      {
        renderer.pipeline_textured->CreateShaderResourceBinding(&batch.srb, true);
        if (batch.srb)
        {
          // Bind the texture
          auto tex_it = renderer.gpu_textures.find(tex_name);
          if (tex_it != renderer.gpu_textures.end() && tex_it->second.srv)
          {
            if (auto* var = batch.srb->GetVariableByName(Diligent::SHADER_TYPE_PIXEL, "g_Texture"))
            {
              var->Set(tex_it->second.srv);
            }
          }
        }
      }

      data.batches.push_back(std::move(batch));
    }

    if (!data.batches.empty())
    {
      renderer.brushes.push_back(std::move(data));
    }
  }
}

void DrawBrushes(
    Diligent::IDeviceContext* context,
    BrushRenderer& renderer,
    const Diligent::float4x4& view_proj)
{
  if (context == nullptr || !renderer.ready)
  {
    return;
  }

  if (renderer.brushes.empty())
  {
    return;
  }

  // Update constant buffer
  {
    Diligent::MapHelper<BrushConstants> constants(
        context,
        renderer.constant_buffer,
        Diligent::MAP_WRITE,
        Diligent::MAP_FLAG_DISCARD);
    constants->view_proj = view_proj;
  }

  // Draw each brush
  for (const BrushRenderData& brush : renderer.brushes)
  {
    for (const BrushFaceBatch& batch : brush.batches)
    {
      if (!batch.vertex_buffer || batch.vertex_count == 0)
      {
        continue;
      }

      // Select pipeline based on whether batch has texture
      if (batch.has_texture && batch.srb && renderer.pipeline_textured)
      {
        context->SetPipelineState(renderer.pipeline_textured);
        context->CommitShaderResources(batch.srb, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
      }
      else if (renderer.pipeline_untextured && renderer.srb_untextured)
      {
        context->SetPipelineState(renderer.pipeline_untextured);
        context->CommitShaderResources(renderer.srb_untextured, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
      }
      else
      {
        continue;
      }

      Diligent::IBuffer* vb = batch.vertex_buffer;
      const Diligent::Uint64 offset = 0;
      context->SetVertexBuffers(
          0, 1, &vb, &offset,
          Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION,
          Diligent::SET_VERTEX_BUFFERS_FLAG_RESET);

      Diligent::DrawAttribs draw_attrs(batch.vertex_count, Diligent::DRAW_FLAG_VERIFY_ALL, 1, 0);
      context->Draw(draw_attrs);
    }
  }
}
