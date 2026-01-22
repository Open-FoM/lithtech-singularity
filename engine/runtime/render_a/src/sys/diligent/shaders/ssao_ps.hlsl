cbuffer SsaoConstants
{
    float2 g_InvTargetSize;
    float2 g_ProjScale;
    float g_Radius;
    float g_Bias;
    float g_Power;
    float g_NearZ;
    float g_FarZ;
    int g_SampleCount;
    float g_Pad0;
    float4 g_Samples[16];
};

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

float Hash(float2 p)
{
    return frac(sin(dot(p, float2(12.9898f, 78.233f))) * 43758.5453f);
}

float4 PSMain(PSInput input) : SV_TARGET
{
    float depth = g_DepthTex.SampleLevel(g_DepthSampler, input.uv, 0.0f);
    if (depth >= 1.0f)
    {
        return float4(1.0f, 1.0f, 1.0f, 1.0f);
    }

    float view_z = LinearizeDepth(depth);
    float2 pixel = input.uv / g_InvTargetSize;
    float rand = Hash(pixel);
    float angle = rand * 6.2831853f;
    float2 rot = float2(cos(angle), sin(angle));

    float occlusion = 0.0f;
    int sample_count = max(g_SampleCount, 1);
    [loop]
    for (int i = 0; i < sample_count; ++i)
    {
        float2 dir = g_Samples[i].xy;
        float2 rdir = float2(dir.x * rot.x - dir.y * rot.y, dir.x * rot.y + dir.y * rot.x);
        float2 offset = 0.5f * rdir * g_ProjScale * (g_Radius / max(view_z, 0.01f));
        float2 sample_uv = saturate(input.uv + offset);
        float sample_depth = g_DepthTex.SampleLevel(g_DepthSampler, sample_uv, 0.0f);
        float sample_z = LinearizeDepth(sample_depth);
        float delta = sample_z - view_z;
        float range_weight = saturate(1.0f - abs(delta) / max(g_Radius, 0.0001f));
        float occluded = (delta < -g_Bias) ? 1.0f : 0.0f;
        occlusion += occluded * range_weight;
    }

    occlusion = 1.0f - occlusion / sample_count;
    occlusion = pow(saturate(occlusion), g_Power);
    return float4(occlusion, occlusion, occlusion, 1.0f);
}
