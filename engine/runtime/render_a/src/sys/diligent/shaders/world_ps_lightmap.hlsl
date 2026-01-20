Texture2D g_Texture0;
Texture2D g_Texture1;
SamplerState g_Texture0_sampler;
SamplerState g_Texture1_sampler;
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

struct PSInput
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

float2 ResolveTexCoord(int stage, float4 coord)
{
    float2 uv = coord.xy;
    int projected = g_TexEffectParams[stage].w;
    int coord_count = g_TexEffectParams[stage].z;
    if (projected != 0)
    {
        float q = (coord_count == 3) ? coord.z : coord.w;
        if (abs(q) > 1e-5f)
        {
            uv /= q;
        }
    }
    return uv;
}

float3 ToLinear(float3 c)
{
    return pow(max(c, 0.0f), 2.2f);
}

float3 ToGamma(float3 c)
{
    return pow(saturate(c), 1.0f / 2.2f);
}

float3 ACESFilm(float3 x)
{
    const float a = 2.51f;
    const float b = 0.03f;
    const float c = 2.43f;
    const float d = 0.59f;
    const float e = 0.14f;
    return saturate((x * (a * x + b)) / (x * (c * x + d) + e));
}

float3 ToneMap(float3 color_linear)
{
    const float exposure = 1.1f;
    return ACESFilm(color_linear * exposure);
}

float3 ApplyToneMap(float3 color_linear)
{
    if (g_FogParams.y > 0.5f)
    {
        return ToneMap(color_linear);
    }
    return color_linear;
}

float3 EncodeOutput(float3 color_linear)
{
    float3 mapped = ApplyToneMap(color_linear);
    return (g_FogParams.z > 0.5f) ? mapped : ToGamma(mapped);
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

float4 ApplyFogLinear(float4 color_linear, float3 world_pos)
{
    if (g_CameraPos.w == 0.0f)
    {
        return color_linear;
    }

    float dist = distance(world_pos, g_CameraPos.xyz);
    float fog_near = g_FogColor.w;
    float fog_far = g_FogParams.x;
    float denom = max(fog_far - fog_near, 0.0001f);
    float t = saturate((dist - fog_near) / denom);
    t = t * t * (3.0f - 2.0f * t);
    float fog_factor = 1.0f - t;
    float3 fog_linear = ToLinear(g_FogColor.xyz);
    float3 mixed = lerp(fog_linear, color_linear.rgb, fog_factor);
    return float4(mixed, color_linear.a);
}

float4 PSMain(PSInput input) : SV_TARGET
{
    if (g_FogParams.w > 3.5f)
    {
        float2 uv0 = input.uv0;
        float4 base_tex = g_Texture0.Sample(g_Texture0_sampler, uv0);
        return float4(base_tex.rgb, 1.0f);
    }
    if (g_FogParams.w > 2.5f)
    {
        float2 uv1 = ResolveTexCoord(1, input.texcoord1);
        float4 lightmap = g_Texture1.Sample(g_Texture1_sampler, uv1);
        return float4(lightmap.rgb, 1.0f);
    }
    if (g_FogParams.w > 1.5f)
    {
        float2 uv0 = ResolveTexCoord(0, input.texcoord0);
        float4 base_tex = g_Texture0.Sample(g_Texture0_sampler, uv0);
        return float4(base_tex.rgb, 1.0f);
    }
    if (g_FogParams.w > 0.5f)
    {
        float2 uv0 = ResolveTexCoord(0, input.texcoord0);
        return float4(frac(uv0.x), frac(uv0.y), 0.0f, 1.0f);
    }

    float2 uv0 = ResolveTexCoord(0, input.texcoord0);
    float2 uv1 = ResolveTexCoord(1, input.texcoord1);
    float4 base_tex = g_Texture0.Sample(g_Texture0_sampler, uv0);
    float4 lightmap = g_Texture1.Sample(g_Texture1_sampler, uv1);
    float3 vertex_color = input.color.rgb;
    float3 color_linear = ToLinear(base_tex.rgb) * ToLinear(lightmap.rgb) * ToLinear(vertex_color);
    float alpha = base_tex.a * lightmap.a * input.color.a;
    float4 fogged = ApplyFogLinear(float4(color_linear, alpha), input.world_pos);

    float3 color = EncodeOutput(fogged.rgb);
    float dither = Dither4x4(input.position.xy) / 255.0f;
    color = saturate(color + dither);

    return float4(color, fogged.a);
}
