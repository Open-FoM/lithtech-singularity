cbuffer DrawPrimConstants
{
    float4x4 g_Mvp;
    float4x4 g_View;
    float4 g_FogColor;
    float4 g_FogParams;
    int g_ColorOp;
    int g_AlphaTestMode;
    int2 g_Padding;
};

struct VSInput
{
    float3 position : ATTRIB0;
    float4 color : ATTRIB1;
    float2 uv : ATTRIB2;
};

struct VSOutput
{
    float4 position : SV_POSITION;
    float4 color : COLOR0;
    float2 uv : TEXCOORD0;
    float3 view_pos : TEXCOORD1;
};

VSOutput VSMain(VSInput input)
{
    VSOutput output;
    float4 world_pos = float4(input.position, 1.0f);
    output.position = mul(g_Mvp, world_pos);
    output.color = input.color;
    output.uv = input.uv;
    output.view_pos = mul(g_View, world_pos).xyz;
    return output;
}
