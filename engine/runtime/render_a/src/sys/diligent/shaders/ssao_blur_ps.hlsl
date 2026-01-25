cbuffer SsaoBlurConstants
{
    float2 g_TexelSize;
    float2 g_Direction;
    float g_Radius;
    float g_NearZ;
    float g_FarZ;
    float g_DepthSigma;
};

Texture2D<float> g_AOTex;
SamplerState g_AOSampler;
Texture2D<float> g_DepthTex;
SamplerState g_DepthSampler;

struct PSInput
{
    float4 position : SV_POSITION;
    float4 color : COLOR0;
    float2 uv : TEXCOORD0;
};

float LinearizeDepth(float depth)
{
    float denom = max(g_FarZ - depth * (g_FarZ - g_NearZ), 0.0001f);
    return (g_FarZ * g_NearZ) / denom;
}

float4 PSMain(PSInput input) : SV_TARGET
{
    float2 dir = g_Direction * g_TexelSize * g_Radius;

    const float w0 = 0.227027f;
    const float w1 = 0.316216f;
    const float w2 = 0.070270f;

    float depth_center = LinearizeDepth(g_DepthTex.SampleLevel(g_DepthSampler, input.uv, 0.0f));
    float sigma = max(g_DepthSigma, 0.0001f);

    float ao = 0.0f;
    float weight_sum = 0.0f;

    float depth = LinearizeDepth(g_DepthTex.SampleLevel(g_DepthSampler, input.uv, 0.0f));
    float weight = w0 * saturate(1.0f - abs(depth - depth_center) / sigma);
    ao += g_AOTex.SampleLevel(g_AOSampler, input.uv, 0.0f).r * weight;
    weight_sum += weight;

    float2 uv1 = input.uv + dir * 1.0f;
    depth = LinearizeDepth(g_DepthTex.SampleLevel(g_DepthSampler, uv1, 0.0f));
    weight = w1 * saturate(1.0f - abs(depth - depth_center) / sigma);
    ao += g_AOTex.SampleLevel(g_AOSampler, uv1, 0.0f).r * weight;
    weight_sum += weight;

    float2 uv2 = input.uv - dir * 1.0f;
    depth = LinearizeDepth(g_DepthTex.SampleLevel(g_DepthSampler, uv2, 0.0f));
    weight = w1 * saturate(1.0f - abs(depth - depth_center) / sigma);
    ao += g_AOTex.SampleLevel(g_AOSampler, uv2, 0.0f).r * weight;
    weight_sum += weight;

    float2 uv3 = input.uv + dir * 2.0f;
    depth = LinearizeDepth(g_DepthTex.SampleLevel(g_DepthSampler, uv3, 0.0f));
    weight = w2 * saturate(1.0f - abs(depth - depth_center) / sigma);
    ao += g_AOTex.SampleLevel(g_AOSampler, uv3, 0.0f).r * weight;
    weight_sum += weight;

    float2 uv4 = input.uv - dir * 2.0f;
    depth = LinearizeDepth(g_DepthTex.SampleLevel(g_DepthSampler, uv4, 0.0f));
    weight = w2 * saturate(1.0f - abs(depth - depth_center) / sigma);
    ao += g_AOTex.SampleLevel(g_AOSampler, uv4, 0.0f).r * weight;
    weight_sum += weight;

    if (weight_sum > 0.0001f)
    {
        ao /= weight_sum;
    }

    return float4(ao, ao, ao, 1.0f);
}
