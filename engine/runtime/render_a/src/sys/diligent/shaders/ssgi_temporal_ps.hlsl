Texture2D g_CurrGITex;
Texture2D g_HistoryTex;
Texture2D g_DepthTex;
Texture2D g_NormalTex;
Texture2D g_MotionTex;
SamplerState g_Sampler;

cbuffer SsgiTemporalConstants
{
    float2 g_InvTargetSize;
    float g_BlendFactor;
    float g_DepthReject;
    float g_NormalReject;
    float pad0[3];
};

struct PSInput
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
};

float4 PSMain(PSInput input) : SV_TARGET
{
    float2 uv = input.uv;
    float3 curr_gi = g_CurrGITex.Sample(g_Sampler, uv).rgb;
    float depth = g_DepthTex.Sample(g_Sampler, uv).r;
    float3 normal = normalize(g_NormalTex.Sample(g_Sampler, uv).xyz);

    float2 motion = g_MotionTex.Sample(g_Sampler, uv).xy;
    float2 history_uv = uv - motion * 0.5f;

    float3 history_gi = g_HistoryTex.Sample(g_Sampler, history_uv).rgb;
    float history_depth = g_DepthTex.Sample(g_Sampler, history_uv).r;
    float3 history_normal = normalize(g_NormalTex.Sample(g_Sampler, history_uv).xyz);

    float depth_diff = abs(history_depth - depth);
    float normal_dot = saturate(dot(normal, history_normal));
    bool reject = (depth_diff > g_DepthReject) || (normal_dot < g_NormalReject);

    float blend = saturate(g_BlendFactor);
    float3 blended = reject ? curr_gi : lerp(curr_gi, history_gi, blend);
    return float4(blended, 1.0f);
}
