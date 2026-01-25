cbuffer SsaoTemporalConstants
{
    float2 g_InvTargetSize;
    float2 g_ProjScale;
    float g_NearZ;
    float g_FarZ;
    float g_BlendFactor;
    float g_VelocityReject;
    float g_DepthReject;
    float g_NormalReject;
    float2 g_DepthTexelSize;
    float4x4 g_InvView;
    float4x4 g_PrevViewProj;
};

Texture2D<float> g_CurrentAOTex;
Texture2D<float> g_HistoryAOTex;
Texture2D<float> g_DepthTex;
Texture2D<float> g_HistoryDepthTex;
SamplerState g_Sampler;
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

float3 ReconstructViewPos(float2 uv, float depth)
{
    float view_z = LinearizeDepth(depth);
    float ndc_x = uv.x * 2.0f - 1.0f;
    float ndc_y = 1.0f - uv.y * 2.0f;
    float view_x = (ndc_x / g_ProjScale.x) * view_z;
    float view_y = (ndc_y / g_ProjScale.y) * view_z;
    return float3(view_x, view_y, view_z);
}

float3 ComputeViewNormal(Texture2D<float> depth_tex, SamplerState depth_sampler, float2 uv)
{
    float depth = depth_tex.SampleLevel(depth_sampler, uv, 0.0f);
    if (depth >= 1.0f)
    {
        return float3(0.0f, 0.0f, 1.0f);
    }

    float depth_dx = depth_tex.SampleLevel(depth_sampler, uv + float2(g_DepthTexelSize.x, 0.0f), 0.0f);
    float depth_dy = depth_tex.SampleLevel(depth_sampler, uv + float2(0.0f, g_DepthTexelSize.y), 0.0f);
    float3 view_pos = ReconstructViewPos(uv, depth);
    float3 view_pos_dx = ReconstructViewPos(uv + float2(g_DepthTexelSize.x, 0.0f), depth_dx);
    float3 view_pos_dy = ReconstructViewPos(uv + float2(0.0f, g_DepthTexelSize.y), depth_dy);

    float3 dpdx = view_pos_dx - view_pos;
    float3 dpdy = view_pos_dy - view_pos;
    float3 normal = normalize(cross(dpdy, dpdx));
    if (normal.z > 0.0f)
    {
        normal = -normal;
    }
    return normal;
}

float4 PSMain(PSInput input) : SV_TARGET
{
    float current = g_CurrentAOTex.SampleLevel(g_Sampler, input.uv, 0.0f).r;
    float depth = g_DepthTex.SampleLevel(g_DepthSampler, input.uv, 0.0f);
    if (depth >= 1.0f)
    {
        return float4(current, current, current, 1.0f);
    }

    float3 view_pos = ReconstructViewPos(input.uv, depth);
    float4 world_pos = mul(g_InvView, float4(view_pos, 1.0f));
    float4 prev_clip = mul(g_PrevViewProj, world_pos);
    if (prev_clip.w <= 0.0001f)
    {
        return float4(current, current, current, 1.0f);
    }

    float2 prev_ndc = prev_clip.xy / prev_clip.w;
    float2 prev_uv = float2(prev_ndc.x * 0.5f + 0.5f, 0.5f - prev_ndc.y * 0.5f);
    if (any(prev_uv < 0.0f) || any(prev_uv > 1.0f))
    {
        return float4(current, current, current, 1.0f);
    }

    float history = g_HistoryAOTex.SampleLevel(g_Sampler, prev_uv, 0.0f).r;
    float min_ao = current;
    float max_ao = current;
    [unroll]
    for (int y = -1; y <= 1; ++y)
    {
        [unroll]
        for (int x = -1; x <= 1; ++x)
        {
            float2 sample_uv = input.uv + float2(x, y) * g_InvTargetSize;
            float sample_ao = g_CurrentAOTex.SampleLevel(g_Sampler, sample_uv, 0.0f).r;
            min_ao = min(min_ao, sample_ao);
            max_ao = max(max_ao, sample_ao);
        }
    }
    history = clamp(history, min_ao, max_ao);

    float2 vel = prev_uv - input.uv;
    float vel_len = length(vel);
    float velocity_weight = 1.0f;
    if (g_VelocityReject > 0.0001f)
    {
        velocity_weight = saturate(1.0f - vel_len / g_VelocityReject);
    }

    float depth_weight = 1.0f;
    if (g_DepthReject > 0.0001f)
    {
        float prev_depth = prev_clip.z / prev_clip.w;
        if (prev_depth <= 0.0f || prev_depth >= 1.0f)
        {
            return float4(current, current, current, 1.0f);
        }
        float history_depth = g_HistoryDepthTex.SampleLevel(g_DepthSampler, prev_uv, 0.0f);
        if (history_depth >= 1.0f)
        {
            return float4(current, current, current, 1.0f);
        }
        float prev_linear = LinearizeDepth(prev_depth);
        float history_linear = LinearizeDepth(history_depth);
        float depth_rel = abs(history_linear - prev_linear) / max(prev_linear, 0.0001f);
        depth_weight = saturate(1.0f - depth_rel / g_DepthReject);
    }

    float normal_weight = 1.0f;
    if (g_NormalReject > 0.0001f)
    {
        float3 normal = ComputeViewNormal(g_DepthTex, g_DepthSampler, input.uv);
        float3 history_normal = ComputeViewNormal(g_HistoryDepthTex, g_DepthSampler, prev_uv);
        float nd = dot(normal, history_normal);
        normal_weight = saturate((nd - g_NormalReject) / max(1.0f - g_NormalReject, 0.0001f));
    }

    float weight = saturate(g_BlendFactor) * velocity_weight * depth_weight * normal_weight;
    float ao = lerp(current, history, weight);
    return float4(ao, ao, ao, 1.0f);
}
