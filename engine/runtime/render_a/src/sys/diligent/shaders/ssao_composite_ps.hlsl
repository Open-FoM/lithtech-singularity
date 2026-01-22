cbuffer SsaoCompositeConstants
{
    float g_Intensity;
    float g_Pad0;
    float2 g_Pad1;
};

Texture2D<float4> g_ColorTex;
Texture2D<float> g_AOTex;
SamplerState g_Sampler;

struct PSInput
{
    float4 position : SV_POSITION;
    float4 color : COLOR0;
    float2 uv : TEXCOORD0;
};

float4 PSMain(PSInput input) : SV_TARGET
{
    float4 color = g_ColorTex.SampleLevel(g_Sampler, input.uv, 0.0f);
    float ao = g_AOTex.SampleLevel(g_Sampler, input.uv, 0.0f).r;
    float occlusion = lerp(1.0f, ao, saturate(g_Intensity));
    return float4(color.rgb * occlusion, color.a);
}
