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
    float g_Pad1;
    float4 g_Samples[16];
    float g_NoiseScale;
    float3 g_Pad2;
    float4x4 g_InvView;
};

Texture2D<float> g_DepthTex;
SamplerState g_DepthSampler;
Texture2D<float> g_BlueNoiseTex;
SamplerState g_BlueNoiseSampler;

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

float3 ReconstructViewPos(float2 uv, float depth)
{
    float view_z = LinearizeDepth(depth);
    float ndc_x = uv.x * 2.0f - 1.0f;
    float ndc_y = 1.0f - uv.y * 2.0f;
    float view_x = (ndc_x / g_ProjScale.x) * view_z;
    float view_y = (ndc_y / g_ProjScale.y) * view_z;
    return float3(view_x, view_y, view_z);
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

    float3 view_pos = ReconstructViewPos(input.uv, depth);
    float2 texel = g_InvTargetSize;
    float depth_dx = g_DepthTex.SampleLevel(g_DepthSampler, input.uv + float2(texel.x, 0.0f), 0.0f);
    float depth_dy = g_DepthTex.SampleLevel(g_DepthSampler, input.uv + float2(0.0f, texel.y), 0.0f);
    float3 view_pos_dx = ReconstructViewPos(input.uv + float2(texel.x, 0.0f), depth_dx);
    float3 view_pos_dy = ReconstructViewPos(input.uv + float2(0.0f, texel.y), depth_dy);

    float3 dpdx = view_pos_dx - view_pos;
    float3 dpdy = view_pos_dy - view_pos;
    float3 normal = normalize(cross(dpdy, dpdx));
    if (normal.z > 0.0f)
    {
        normal = -normal;
    }

    float3 world_pos = mul(g_InvView, float4(view_pos, 1.0f)).xyz;
    float2 noise_pos = world_pos.xz * g_NoiseScale;
    float2 noise_uv = frac(noise_pos);
    float rand = g_BlueNoiseTex.SampleLevel(g_BlueNoiseSampler, noise_uv, 0.0f).r;
    if (rand <= 0.0001f || rand >= 0.9999f)
    {
        rand = Hash(noise_pos);
    }
    float angle = rand * 6.2831853f;
    float2 rot = float2(cos(angle), sin(angle));

    float3 up = abs(normal.y) < 0.999f ? float3(0.0f, 1.0f, 0.0f) : float3(1.0f, 0.0f, 0.0f);
    float3 tangent = normalize(cross(up, normal));
    float3 bitangent = cross(normal, tangent);
    float3 t_rot = tangent * rot.x + bitangent * rot.y;
    float3 b_rot = -tangent * rot.y + bitangent * rot.x;

    float occlusion = 0.0f;
    int sample_count = max(g_SampleCount, 1);
    [loop]
    for (int i = 0; i < sample_count; ++i)
    {
        float3 s = g_Samples[i].xyz;
        float3 sample_dir = t_rot * s.x + b_rot * s.y + normal * s.z;
        float3 sample_pos = view_pos + sample_dir * g_Radius;
        if (sample_pos.z <= 0.0001f)
        {
            continue;
        }

        float2 sample_ndc = float2(
            (sample_pos.x / sample_pos.z) * g_ProjScale.x,
            (sample_pos.y / sample_pos.z) * g_ProjScale.y);
        float2 sample_uv = float2(sample_ndc.x * 0.5f + 0.5f, 0.5f - sample_ndc.y * 0.5f);
        if (any(sample_uv < 0.0f) || any(sample_uv > 1.0f))
        {
            continue;
        }

        float sample_depth = g_DepthTex.SampleLevel(g_DepthSampler, sample_uv, 0.0f);
        float sample_z = LinearizeDepth(sample_depth);
        float delta = sample_z - view_pos.z;
        float range_weight = saturate(1.0f - abs(delta) / max(g_Radius, 0.0001f));
        float occluded = (sample_z < sample_pos.z - g_Bias) ? 1.0f : 0.0f;
        occlusion += occluded * range_weight;
    }

    occlusion = 1.0f - occlusion / sample_count;
    occlusion = pow(saturate(occlusion), g_Power);
    return float4(occlusion, occlusion, occlusion, 1.0f);
}
