Texture2D g_GITex;
SamplerState g_Sampler;

struct PSInput
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
};

float4 PSMain(PSInput input) : SV_TARGET
{
    float3 gi = g_GITex.Sample(g_Sampler, input.uv).rgb;
    return float4(gi, 1.0f);
}
