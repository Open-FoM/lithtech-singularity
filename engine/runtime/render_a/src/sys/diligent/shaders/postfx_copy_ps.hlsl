Texture2D g_ColorTex;
SamplerState g_Sampler;

struct PSInput
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
};

float4 PSMain(PSInput input) : SV_TARGET
{
    return g_ColorTex.Sample(g_Sampler, input.uv);
}
