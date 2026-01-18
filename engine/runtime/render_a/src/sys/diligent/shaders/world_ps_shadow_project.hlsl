Texture2D g_ShadowTexture;
SamplerState g_ShadowTexture_sampler;
cbuffer ShadowProjectConstants
{
    float4x4 g_WorldToShadow;
    float4 g_LightDir;
    float4 g_ProjCenter;
};

struct PSInput
{
    float4 position : SV_POSITION;
    float4 color : COLOR0;
    float2 uv0 : TEXCOORD0;
    float2 uv1 : TEXCOORD1;
    float3 world_pos : TEXCOORD2;
    float3 world_normal : TEXCOORD3;
    float4 texcoord0 : TEXCOORD4;
    float4 texcoord1 : TEXCOORD5;
    float4 texcoord2 : TEXCOORD6;
    float4 texcoord3 : TEXCOORD7;
};

float4 PSMain(PSInput input) : SV_TARGET
{
    float4 shadow_pos = mul(g_WorldToShadow, float4(input.world_pos, 1.0f));
    float2 uv = shadow_pos.xy / max(shadow_pos.w, 0.0001f);
    float shadow = g_ShadowTexture.Sample(g_ShadowTexture_sampler, uv).r;
    return float4(shadow, shadow, shadow, 1.0f);
}
