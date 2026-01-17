Texture2D g_Texture0;
Texture2D g_Texture1;
SamplerState g_Texture0_sampler;
SamplerState g_Texture1_sampler;
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
