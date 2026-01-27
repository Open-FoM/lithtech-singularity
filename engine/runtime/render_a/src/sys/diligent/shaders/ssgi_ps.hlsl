Texture2D g_ColorTex;
Texture2D g_NormalTex;
Texture2D g_DepthTex;
Texture2D g_RoughnessTex;
SamplerState g_Sampler;

cbuffer SsgiConstants
{
    float2 g_InvTargetSize;
    float g_Radius;
    float g_Intensity;
    float g_DepthReject;
    float g_NormalReject;
    int g_SampleCount;
    float pad0;
    float pad1;
};

struct PSInput
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
};

static const float2 kOffsets[8] =
{
    float2(1.0f, 0.0f),
    float2(-1.0f, 0.0f),
    float2(0.0f, 1.0f),
    float2(0.0f, -1.0f),
    float2(0.7071f, 0.7071f),
    float2(-0.7071f, 0.7071f),
    float2(0.7071f, -0.7071f),
    float2(-0.7071f, -0.7071f)
};

float4 PSMain(PSInput input) : SV_TARGET
{
    float2 uv = input.uv;
    float3 normal = g_NormalTex.Sample(g_Sampler, uv).xyz;
    normal = normalize(normal);
    float depth = g_DepthTex.Sample(g_Sampler, uv).r;
    float roughness = saturate(g_RoughnessTex.Sample(g_Sampler, uv).r);

    float2 radius_uv = g_Radius * g_InvTargetSize;
    int sample_count = clamp(g_SampleCount, 1, 8);

    float3 accum = 0.0f.xxx;
    float accum_weight = 0.0f;

    [unroll]
    for (int i = 0; i < 8; ++i)
    {
        if (i >= sample_count)
        {
            break;
        }

        float2 sample_uv = uv + kOffsets[i] * radius_uv;
        float sample_depth = g_DepthTex.Sample(g_Sampler, sample_uv).r;
        float depth_diff = abs(sample_depth - depth);
        float depth_weight = saturate(1.0f - (depth_diff / max(g_DepthReject, 1e-4f)));
        if (depth_weight <= 0.0f)
        {
            continue;
        }

        float3 sample_normal = g_NormalTex.Sample(g_Sampler, sample_uv).xyz;
        sample_normal = normalize(sample_normal);
        float normal_dot = saturate(dot(normal, sample_normal));
        float normal_weight = saturate((normal_dot - g_NormalReject) / max(1.0f - g_NormalReject, 1e-3f));
        float weight = depth_weight * normal_weight;
        if (weight <= 0.0f)
        {
            continue;
        }

        float3 sample_color = g_ColorTex.Sample(g_Sampler, sample_uv).rgb;
        accum += sample_color * weight;
        accum_weight += weight;
    }

    float3 gi = (accum_weight > 1e-4f) ? (accum / accum_weight) : 0.0f.xxx;
    gi *= g_Intensity * (1.0f - roughness);
    return float4(gi, 1.0f);
}
