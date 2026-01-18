cbuffer WorldConstants
{
    float4x4 g_Mvp;
    float4x4 g_View;
    float4x4 g_World;
    float4 g_CameraPos;
    float4 g_FogColor;
    float4 g_FogParams;
    float4 g_DynamicLightPos;
    float4 g_DynamicLightColor;
    float4x4 g_TexEffectMatrix[4];
    int4 g_TexEffectParams[4];
    int4 g_TexEffectUV[4];
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
    float3 light_pos = g_DynamicLightPos.xyz;
    float radius = g_DynamicLightPos.w;
    float dist = distance(input.world_pos, light_pos);
    float intensity = saturate(1.0f - dist / max(radius, 0.0001f));
    float3 light_color = g_DynamicLightColor.xyz * intensity;
    return float4(light_color, 1.0f) * input.color;
}
