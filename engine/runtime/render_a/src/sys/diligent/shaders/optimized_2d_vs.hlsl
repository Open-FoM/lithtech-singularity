struct VSInput
{
    float2 position : ATTRIB0;
    float4 color : ATTRIB1;
    float2 uv : ATTRIB2;
};

struct VSOutput
{
    float4 position : SV_POSITION;
    float4 color : COLOR0;
    float2 uv : TEXCOORD0;
};

VSOutput VSMain(VSInput input)
{
    VSOutput output;
    output.position = float4(input.position, 0.0f, 1.0f);
    output.color = input.color;
    output.uv = input.uv;
    return output;
}
