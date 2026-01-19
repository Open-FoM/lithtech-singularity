Texture2D g_Texture;
SamplerState g_Texture_sampler;

cbuffer Optimized2DConstants
{
    float4 g_OutputParams;
};

struct PSInput
{
    float4 position : SV_POSITION;
    float4 color : COLOR0;
    float2 uv : TEXCOORD0;
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
    return (g_OutputParams.x > 0.5f) ? color_linear : gamma;
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

float4 PSMain(PSInput input) : SV_TARGET
{
    float4 tex = g_Texture.Sample(g_Texture_sampler, input.uv);

    float3 color_linear = ToLinear(tex.rgb) * ToLinear(input.color.rgb);
    float alpha = tex.a * input.color.a;

    float3 color = EncodeOutput(color_linear);

    float dither = Dither4x4(input.position.xy) / 255.0f;
    color = saturate(color + dither);

    return float4(color, alpha);
}
