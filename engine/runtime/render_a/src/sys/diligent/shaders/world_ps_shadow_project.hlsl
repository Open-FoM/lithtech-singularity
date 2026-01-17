Texture2D g_ShadowTexture;
SamplerState g_ShadowTexture_sampler;
cbuffer ShadowProjectConstants
{
    float4x4 g_WorldToShadow;
    float4 g_LightDir;
    float4 g_ProjCenter;
