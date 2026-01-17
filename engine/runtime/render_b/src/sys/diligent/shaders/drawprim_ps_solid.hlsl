cbuffer DrawPrimConstants
{
    float4x4 g_Mvp;
    float4x4 g_View;
    float4 g_FogColor;
    float4 g_FogParams;
    int g_ColorOp;
    int g_AlphaTestMode;
    int2 g_Padding;
