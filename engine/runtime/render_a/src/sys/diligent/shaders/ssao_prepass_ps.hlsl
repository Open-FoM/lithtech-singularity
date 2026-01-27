struct PSInput
{
    float4 position : SV_POSITION;
    float3 world_normal : TEXCOORD0;
    float4 curr_clip : TEXCOORD1;
    float4 prev_clip : TEXCOORD2;
};

cbuffer SsaoPrepassConstants
{
    float4x4 g_ViewProj;
    float4x4 g_PrevViewProj;
    float4x4 g_World;
    float4x4 g_PrevWorld;
    float4 g_PrepassParams;
};

struct PSOutput
{
    float4 normal : SV_Target0;
    float2 motion : SV_Target1;
    float roughness : SV_Target2;
};

float SafeDiv(float num, float denom)
{
    return num / ((abs(denom) < 1e-6f) ? ((denom < 0.0f) ? -1e-6f : 1e-6f) : denom);
}

PSOutput PSMain(PSInput input)
{
    PSOutput output;
    float3 normal = normalize(input.world_normal);
    output.normal = float4(normal, 0.0f);

    float2 curr_ndc = float2(
        SafeDiv(input.curr_clip.x, input.curr_clip.w),
        SafeDiv(input.curr_clip.y, input.curr_clip.w));
    float2 prev_ndc = float2(
        SafeDiv(input.prev_clip.x, input.prev_clip.w),
        SafeDiv(input.prev_clip.y, input.prev_clip.w));
    output.motion = curr_ndc - prev_ndc;
    output.roughness = saturate(g_PrepassParams.x);
    return output;
}
