cbuffer SsaoPrepassConstants
{
    float4x4 g_ViewProj;
    float4x4 g_PrevViewProj;
    float4x4 g_World;
    float4x4 g_PrevWorld;
};

struct VSInput
{
    float3 position : ATTRIB0;
    float4 color : ATTRIB1;
    float2 uv0 : ATTRIB2;
    float2 uv1 : ATTRIB3;
    float3 normal : ATTRIB4;
};

struct VSOutput
{
    float4 position : SV_POSITION;
    float3 world_normal : TEXCOORD0;
    float4 curr_clip : TEXCOORD1;
    float4 prev_clip : TEXCOORD2;
};

VSOutput VSMain(VSInput input)
{
    VSOutput output;

    float4 world_pos = mul(g_World, float4(input.position, 1.0f));
    float4 prev_world_pos = mul(g_PrevWorld, float4(input.position, 1.0f));

    float4 curr_clip = mul(g_ViewProj, world_pos);
    float4 prev_clip = mul(g_PrevViewProj, prev_world_pos);

    output.position = curr_clip;
    output.curr_clip = curr_clip;
    output.prev_clip = prev_clip;

    float3 world_normal = normalize(mul((float3x3)g_World, input.normal));
    output.world_normal = world_normal;

    return output;
}
