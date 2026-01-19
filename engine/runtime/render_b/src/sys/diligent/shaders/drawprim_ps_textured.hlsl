#define DRAWPRIM_NOCOLOROP 0
#define DRAWPRIM_MODULATE 1
#define DRAWPRIM_ADD 2
#define DRAWPRIM_DECAL 3

#define DRAWPRIM_NOALPHATEST 0
#define DRAWPRIM_ALPHATEST_LESS 1
#define DRAWPRIM_ALPHATEST_LESSEQUAL 2
#define DRAWPRIM_ALPHATEST_GREATER 3
#define DRAWPRIM_ALPHATEST_GREATEREQUAL 4
#define DRAWPRIM_ALPHATEST_EQUAL 5
#define DRAWPRIM_ALPHATEST_NOTEQUAL 6

Texture2D g_Texture;
SamplerState g_Texture_sampler;

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

struct PSInput
{
    float4 position : SV_POSITION;
    float4 color : COLOR0;
    float2 uv : TEXCOORD0;
    float3 view_pos : TEXCOORD1;
};

float3 ToLinear(float3 c)
{
    return pow(max(c, 0.0f), 2.2f);
}

float3 ToGamma(float3 c)
{
    return pow(saturate(c), 1.0f / 2.2f);
}

float3 EncodeOutput(float3 color_linear)
{
    float3 gamma = ToGamma(color_linear);
    return (g_FogParams.w > 0.5f) ? color_linear : gamma;
}

float Dither4x4(float2 pos)
{
    int2 p = int2(pos);
    int x = p.x & 3;
    int y = p.y & 3;
    int idx = (y << 2) | x;
    static const float bayer[16] =
    {
        0.0f, 8.0f, 2.0f, 10.0f,
        12.0f, 4.0f, 14.0f, 6.0f,
        3.0f, 11.0f, 1.0f, 9.0f,
        15.0f, 7.0f, 13.0f, 5.0f
    };
    return (bayer[idx] + 0.5f) / 16.0f - 0.5f;
}

float4 ApplyFogLinear(float4 color_linear, float3 view_pos)
{
    if (g_FogParams.z == 0.0f)
    {
        return color_linear;
    }

    float dist = length(view_pos);
    float fog_near = g_FogParams.x;
    float fog_far = g_FogParams.y;
    float denom = max(fog_far - fog_near, 0.0001f);
    float t = saturate((dist - fog_near) / denom);
    t = t * t * (3.0f - 2.0f * t);
    float fog_factor = 1.0f - t;
    float3 fog_linear = ToLinear(g_FogColor.xyz);
    float3 mixed = lerp(fog_linear, color_linear.rgb, fog_factor);
    return float4(mixed, color_linear.a);
}

float4 ApplyColorOp(float4 diffuse_linear, float4 texel_linear)
{
    if (g_ColorOp == DRAWPRIM_NOCOLOROP)
    {
        return diffuse_linear;
    }
    if (g_ColorOp == DRAWPRIM_ADD)
    {
        return diffuse_linear + texel_linear;
    }
    if (g_ColorOp == DRAWPRIM_DECAL)
    {
        return texel_linear;
    }
    return diffuse_linear * texel_linear;
}

bool AlphaTestFail(float alpha, float tex_alpha)
{
    if (g_AlphaTestMode == DRAWPRIM_NOALPHATEST)
    {
        return false;
    }

    if (g_AlphaTestMode == DRAWPRIM_ALPHATEST_LESS)
    {
        return !(alpha < tex_alpha);
    }
    if (g_AlphaTestMode == DRAWPRIM_ALPHATEST_LESSEQUAL)
    {
        return !(alpha <= tex_alpha);
    }
    if (g_AlphaTestMode == DRAWPRIM_ALPHATEST_GREATER)
    {
        return !(alpha > tex_alpha);
    }
    if (g_AlphaTestMode == DRAWPRIM_ALPHATEST_GREATEREQUAL)
    {
        return !(alpha >= tex_alpha);
    }
    if (g_AlphaTestMode == DRAWPRIM_ALPHATEST_EQUAL)
    {
        return !(abs(alpha - tex_alpha) <= 0.0001f);
    }
    if (g_AlphaTestMode == DRAWPRIM_ALPHATEST_NOTEQUAL)
    {
        return !(abs(alpha - tex_alpha) > 0.0001f);
    }

    return false;
}

float4 PSMain(PSInput input) : SV_TARGET
{
    float4 tex = g_Texture.Sample(g_Texture_sampler, input.uv);

    if (AlphaTestFail(input.color.a, tex.a))
    {
        discard;
    }

    float4 diffuse_linear = float4(ToLinear(input.color.rgb), input.color.a);
    float4 texel_linear = float4(ToLinear(tex.rgb), tex.a);
    float4 combined = ApplyColorOp(diffuse_linear, texel_linear);

    float4 fogged = ApplyFogLinear(combined, input.view_pos);
    float3 color = EncodeOutput(fogged.rgb);

    float dither = Dither4x4(input.position.xy) / 255.0f;
    color = saturate(color + dither);

    return float4(color, fogged.a);
}
