cbuffer SsaoBlurConstants
{
    float2 g_TexelSize;
    float2 g_Direction;
    float g_Radius;
    float g_Pad0;
};

Texture2D<float> g_AOTex;
SamplerState g_AOSampler;

struct PSInput
{
    float4 position : SV_POSITION;
    float4 color : COLOR0;
    float2 uv : TEXCOORD0;
};

float4 PSMain(PSInput input) : SV_TARGET
{
    float2 dir = g_Direction * g_TexelSize * g_Radius;

    const float w0 = 0.227027f;
    const float w1 = 0.316216f;
    const float w2 = 0.070270f;

    float ao = g_AOTex.SampleLevel(g_AOSampler, input.uv, 0.0f).r * w0;
    ao += g_AOTex.SampleLevel(g_AOSampler, input.uv + dir * 1.0f, 0.0f).r * w1;
    ao += g_AOTex.SampleLevel(g_AOSampler, input.uv - dir * 1.0f, 0.0f).r * w1;
    ao += g_AOTex.SampleLevel(g_AOSampler, input.uv + dir * 2.0f, 0.0f).r * w2;
    ao += g_AOTex.SampleLevel(g_AOSampler, input.uv - dir * 2.0f, 0.0f).r * w2;

    return float4(ao, ao, ao, 1.0f);
}
