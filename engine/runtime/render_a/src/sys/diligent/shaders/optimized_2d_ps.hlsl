Texture2D g_Texture;
SamplerState g_Texture_sampler;
struct PSInput
{
    float4 position : SV_POSITION;
    float4 color : COLOR0;
    float2 uv : TEXCOORD0;
    float3 world_pos : TEXCOORD1;
