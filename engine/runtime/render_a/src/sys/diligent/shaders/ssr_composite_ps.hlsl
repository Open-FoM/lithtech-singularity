Texture2D g_ColorTex;
Texture2D g_SSRColorTex;
SamplerState g_Sampler;

cbuffer SsrCompositeConstants
{
    float g_Intensity;
    float pad0[3];
};

struct PSInput
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
};

float4 PSMain(PSInput input) : SV_TARGET
{
    float3 color = g_ColorTex.Sample(g_Sampler, input.uv).rgb;
    float4 ssr = g_SSRColorTex.Sample(g_Sampler, input.uv);
    float3 combined = color + (ssr.rgb * ssr.a * g_Intensity);
    return float4(combined, 1.0f);
}
