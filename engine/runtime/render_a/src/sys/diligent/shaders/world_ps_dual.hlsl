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

float4 ApplyFog(float4 color, float3 world_pos)
{
    if (g_CameraPos.w == 0.0f)
    {
        return color;
    }

    float dist = distance(world_pos, g_CameraPos.xyz);
    float fog_near = g_FogColor.w;
    float fog_far = g_FogParams.x;
    float denom = max(fog_far - fog_near, 0.0001f);
    float fog_factor = saturate((fog_far - dist) / denom);
    float4 fog_color = float4(g_FogColor.xyz, color.a);
    return lerp(fog_color, color, fog_factor);
}

float4 PSMain(PSInput input) : SV_TARGET
{
    float2 uv0 = ResolveTexCoord(0, input.texcoord0);
    float2 uv1 = ResolveTexCoord(1, input.texcoord1);
    float4 base_tex = g_Texture0.Sample(g_Texture0_sampler, uv0);
    float4 dual_tex = g_Texture1.Sample(g_Texture1_sampler, uv1);
    float4 color = base_tex * dual_tex * input.color;
    return ApplyFog(color, input.world_pos);
}
