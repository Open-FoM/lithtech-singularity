#define GLOW_MAX_TAPS 8
Texture2D g_Texture;
SamplerState g_Texture_sampler;

cbuffer GlowBlurConstants
{
    float4 g_Taps[GLOW_MAX_TAPS];
    float4 g_Params;
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
    int tap_count = (int)g_Params.x;
    tap_count = min(tap_count, GLOW_MAX_TAPS);

    float3 sum = float3(0.0f, 0.0f, 0.0f);
    float weight_sum = 0.0f;

    [unroll]
    for (int i = 0; i < GLOW_MAX_TAPS; ++i)
    {
        if (i >= tap_count)
        {
            break;
        }
        float4 tap = g_Taps[i];
        float2 uv = input.uv + tap.xy;
        float3 sample_color = ToLinear(g_Texture.Sample(g_Texture_sampler, uv).rgb);
        sum += sample_color * tap.z;
        weight_sum += tap.z;
    }

    if (weight_sum > 0.0001f)
    {
        sum /= weight_sum;
    }

    float3 color_linear = sum * ToLinear(input.color.rgb);
    float3 color = ToGamma(color_linear);

    float dither = Dither4x4(input.position.xy) / 255.0f;
    color = saturate(color + dither);

    return float4(color, input.color.a);
}
