cbuffer WorldConstants
{
    float4x4 g_Mvp;
    float4x4 g_View;
    float4x4 g_World;
    float4 g_CameraPos;
    float4 g_FogColor;
    float4 g_FogParams;
    float4 g_DynamicLightPos;
    float4 g_DynamicLightColor;
    float4x4 g_TexEffectMatrix[4];
    int4 g_TexEffectParams[4];
    int4 g_TexEffectUV[4];
};

#define TEXINPUT_POS 0
#define TEXINPUT_NORMAL 1
#define TEXINPUT_REFLECTION 2
#define TEXINPUT_UV 3

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
    float4 color : COLOR0;
    float2 uv0 : TEXCOORD0;
    float2 uv1 : TEXCOORD1;
    float3 world_pos : TEXCOORD2;
    float3 world_normal : TEXCOORD3;
    float4 texcoord0 : TEXCOORD4;
    float4 texcoord1 : TEXCOORD5;
    float4 texcoord2 : TEXCOORD6;
    float4 texcoord3 : TEXCOORD7;
};

float4 BuildTexCoord(int stage, float2 uv0, float2 uv1, float3 view_pos, float3 view_normal, float3 view_reflect)
{
    int input_type = g_TexEffectParams[stage].y;
    int use_uv1 = g_TexEffectUV[stage].x;
    float2 uv = (use_uv1 != 0) ? uv1 : uv0;
    if (input_type == TEXINPUT_UV)
    {
        return float4(uv, 0.0f, 1.0f);
    }
    if (input_type == TEXINPUT_POS)
    {
        return float4(view_pos, 1.0f);
    }
    if (input_type == TEXINPUT_NORMAL)
    {
        return float4(view_normal, 0.0f);
    }
    if (input_type == TEXINPUT_REFLECTION)
    {
        return float4(view_reflect, 0.0f);
    }
    return float4(uv, 0.0f, 1.0f);
}

float4 TransformTexCoord(int stage, float4 base_coord)
{
    if (g_TexEffectParams[stage].x == 0)
    {
        return base_coord;
    }

    return mul(g_TexEffectMatrix[stage], base_coord);
}

VSOutput VSMain(VSInput input)
{
    VSOutput output;

    float4 world_pos = mul(g_World, float4(input.position, 1.0f));
    output.position = mul(g_Mvp, float4(input.position, 1.0f));
    output.color = input.color;
    output.uv0 = input.uv0;
    output.uv1 = input.uv1;
    output.world_pos = world_pos.xyz;

    float3 world_normal = normalize(mul((float3x3)g_World, input.normal));
    output.world_normal = world_normal;

    float3 view_pos = mul(g_View, world_pos).xyz;
    float3 view_normal = normalize(mul((float3x3)g_View, world_normal));
    float3 view_dir = normalize(-view_pos);
    float3 view_reflect = reflect(view_dir, view_normal);

    float4 base0 = BuildTexCoord(0, input.uv0, input.uv1, view_pos, view_normal, view_reflect);
    float4 base1 = BuildTexCoord(1, input.uv0, input.uv1, view_pos, view_normal, view_reflect);
    float4 base2 = BuildTexCoord(2, input.uv0, input.uv1, view_pos, view_normal, view_reflect);
    float4 base3 = BuildTexCoord(3, input.uv0, input.uv1, view_pos, view_normal, view_reflect);

    output.texcoord0 = TransformTexCoord(0, base0);
    output.texcoord1 = TransformTexCoord(1, base1);
    output.texcoord2 = TransformTexCoord(2, base2);
    output.texcoord3 = TransformTexCoord(3, base3);

    return output;
}
