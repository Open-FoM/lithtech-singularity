#define GLOW_MAX_TAPS 8
Texture2D g_Texture;
SamplerState g_Texture_sampler;
cbuffer GlowBlurConstants
{
    float4 g_Taps[GLOW_MAX_TAPS];
    float4 g_Params;
