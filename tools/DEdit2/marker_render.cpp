#include "marker_render.h"

#include "DiligentCore/Graphics/GraphicsTools/interface/MapHelper.hpp"

#include <vector>

namespace {

struct MarkerVertex
{
  Diligent::float3 position;
  Diligent::float4 color;
};

struct MarkerConstants
{
  Diligent::float4x4 view_proj;
  Diligent::float3 marker_position;
  float scale;
};

void BuildMarkerVertices(std::vector<MarkerVertex>& out_vertices)
{
  out_vertices.clear();

  // Marker color: bright yellow
  const Diligent::float4 color(1.0f, 0.9f, 0.2f, 1.0f);

  // Create a 3D crosshair with lines along each axis
  // Vertices are in local space, will be scaled and translated in shader
  const float size = 1.0f; // Unit size, scaled in shader

  // X axis line
  out_vertices.push_back({Diligent::float3(-size, 0.0f, 0.0f), color});
  out_vertices.push_back({Diligent::float3(size, 0.0f, 0.0f), color});

  // Y axis line
  out_vertices.push_back({Diligent::float3(0.0f, -size, 0.0f), color});
  out_vertices.push_back({Diligent::float3(0.0f, size, 0.0f), color});

  // Z axis line
  out_vertices.push_back({Diligent::float3(0.0f, 0.0f, -size), color});
  out_vertices.push_back({Diligent::float3(0.0f, 0.0f, size), color});
}

} // namespace

bool InitMarkerRenderer(
    Diligent::IRenderDevice* device,
    Diligent::TEXTURE_FORMAT rtv_format,
    Diligent::TEXTURE_FORMAT dsv_format,
    MarkerRenderer& renderer)
{
  if (device == nullptr)
  {
    return false;
  }

  std::vector<MarkerVertex> vertices;
  BuildMarkerVertices(vertices);
  if (vertices.empty())
  {
    return false;
  }

  renderer.vertex_count = static_cast<uint32_t>(vertices.size());

  // Create vertex buffer
  Diligent::BufferDesc vb_desc;
  vb_desc.Name = "DEdit2 Marker VB";
  vb_desc.Usage = Diligent::USAGE_IMMUTABLE;
  vb_desc.BindFlags = Diligent::BIND_VERTEX_BUFFER;
  vb_desc.Size = static_cast<Diligent::Uint64>(vertices.size() * sizeof(MarkerVertex));

  Diligent::BufferData vb_data;
  vb_data.pData = vertices.data();
  vb_data.DataSize = vb_desc.Size;
  device->CreateBuffer(vb_desc, &vb_data, &renderer.vertex_buffer);
  if (!renderer.vertex_buffer)
  {
    return false;
  }

  // Create constant buffer
  Diligent::BufferDesc cb_desc;
  cb_desc.Name = "DEdit2 Marker CB";
  cb_desc.Usage = Diligent::USAGE_DYNAMIC;
  cb_desc.BindFlags = Diligent::BIND_UNIFORM_BUFFER;
  cb_desc.CPUAccessFlags = Diligent::CPU_ACCESS_WRITE;
  cb_desc.Size = sizeof(MarkerConstants);
  device->CreateBuffer(cb_desc, nullptr, &renderer.constant_buffer);
  if (!renderer.constant_buffer)
  {
    return false;
  }

  // Vertex shader: scales local vertices and translates to marker position
  static const char* vs_source = R"(
cbuffer MarkerData : register(b0)
{
    float4x4 g_ViewProj;
    float3 g_MarkerPosition;
    float g_Scale;
};

struct VSInput
{
    float3 Pos   : ATTRIB0;
    float4 Color : ATTRIB1;
};

struct PSInput
{
    float4 Pos   : SV_POSITION;
    float4 Color : COLOR0;
};

PSInput main(VSInput input)
{
    PSInput output;
    // Scale local vertex and translate to marker world position
    float3 worldPos = input.Pos * g_Scale + g_MarkerPosition;
    output.Pos = mul(g_ViewProj, float4(worldPos, 1.0f));
    output.Color = input.Color;
    return output;
}
)";

  // Pixel shader: simple color output
  static const char* ps_source = R"(
struct PSInput
{
    float4 Pos   : SV_POSITION;
    float4 Color : COLOR0;
};

float4 main(PSInput input) : SV_TARGET
{
    return input.Color;
}
)";

  Diligent::ShaderCreateInfo shader_ci;
  shader_ci.SourceLanguage = Diligent::SHADER_SOURCE_LANGUAGE_HLSL;

  Diligent::RefCntAutoPtr<Diligent::IShader> vs;
  shader_ci.Desc.ShaderType = Diligent::SHADER_TYPE_VERTEX;
  shader_ci.Desc.Name = "DEdit2 Marker VS";
  shader_ci.EntryPoint = "main";
  shader_ci.Source = vs_source;
  device->CreateShader(shader_ci, &vs);
  if (!vs)
  {
    return false;
  }

  Diligent::RefCntAutoPtr<Diligent::IShader> ps;
  shader_ci.Desc.ShaderType = Diligent::SHADER_TYPE_PIXEL;
  shader_ci.Desc.Name = "DEdit2 Marker PS";
  shader_ci.EntryPoint = "main";
  shader_ci.Source = ps_source;
  device->CreateShader(shader_ci, &ps);
  if (!ps)
  {
    return false;
  }

  // Create pipeline state
  Diligent::GraphicsPipelineStateCreateInfo pso_ci;
  pso_ci.PSODesc.Name = "DEdit2 Marker PSO";
  pso_ci.PSODesc.ResourceLayout.DefaultVariableType = Diligent::SHADER_RESOURCE_VARIABLE_TYPE_STATIC;
  pso_ci.pVS = vs;
  pso_ci.pPS = ps;

  auto& gp = pso_ci.GraphicsPipeline;
  gp.PrimitiveTopology = Diligent::PRIMITIVE_TOPOLOGY_LINE_LIST;
  gp.NumRenderTargets = 1;
  gp.RTVFormats[0] = rtv_format;
  gp.DSVFormat = dsv_format;
  gp.RasterizerDesc.CullMode = Diligent::CULL_MODE_NONE;
  // Disable depth test so marker is always visible
  gp.DepthStencilDesc.DepthEnable = false;
  gp.DepthStencilDesc.DepthWriteEnable = false;

  Diligent::LayoutElement layout[] = {
      Diligent::LayoutElement{0, 0, 3, Diligent::VT_FLOAT32, false},
      Diligent::LayoutElement{1, 0, 4, Diligent::VT_FLOAT32, false}};
  gp.InputLayout.LayoutElements = layout;
  gp.InputLayout.NumElements = static_cast<Diligent::Uint32>(sizeof(layout) / sizeof(layout[0]));

  device->CreateGraphicsPipelineState(pso_ci, &renderer.pipeline);
  if (!renderer.pipeline)
  {
    return false;
  }

  if (auto* var = renderer.pipeline->GetStaticVariableByName(Diligent::SHADER_TYPE_VERTEX, "MarkerData"))
  {
    var->Set(renderer.constant_buffer);
  }
  renderer.pipeline->CreateShaderResourceBinding(&renderer.srb, true);
  return renderer.srb != nullptr;
}

void UpdateMarkerConstants(
    Diligent::IDeviceContext* context,
    MarkerRenderer& renderer,
    const float marker_position[3],
    const Diligent::float4x4& view_proj,
    float scale)
{
  if (context == nullptr || !renderer.constant_buffer)
  {
    return;
  }

  Diligent::MapHelper<MarkerConstants> constants(context, renderer.constant_buffer,
                                                  Diligent::MAP_WRITE, Diligent::MAP_FLAG_DISCARD);
  constants->view_proj = view_proj;
  constants->marker_position = Diligent::float3(marker_position[0], marker_position[1], marker_position[2]);
  constants->scale = scale;
}

void DrawMarker(
    Diligent::IDeviceContext* context,
    MarkerRenderer& renderer,
    bool visible)
{
  if (context == nullptr || !renderer.pipeline || !renderer.vertex_buffer)
  {
    return;
  }
  if (!visible || renderer.vertex_count == 0)
  {
    return;
  }

  context->SetPipelineState(renderer.pipeline);
  Diligent::IBuffer* vb = renderer.vertex_buffer;
  const Diligent::Uint64 offset = 0;
  context->SetVertexBuffers(0, 1, &vb, &offset, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION,
                            Diligent::SET_VERTEX_BUFFERS_FLAG_RESET);
  context->CommitShaderResources(renderer.srb, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

  Diligent::DrawAttribs draw_attrs(renderer.vertex_count, Diligent::DRAW_FLAG_VERIFY_ALL, 1, 0);
  context->Draw(draw_attrs);
}
