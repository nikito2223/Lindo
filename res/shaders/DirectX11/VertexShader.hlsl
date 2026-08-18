cbuffer Matrices : register(b0)
{
    float4x4 model;
    float4x4 view;
    float4x4 projection;
    float3x3 normalMatrix;
}

struct VSInput
{
    float3 position : POSITION;
    float3 normal   : NORMAL;
    float2 texcoord : TEXCOORD0;
};

struct VSOutput
{
    float4 position : SV_POSITION;
    float3 FragPos  : TEXCOORD0;
    float3 Normal   : TEXCOORD1;
    float2 TexCoords: TEXCOORD2;
};

VSOutput main(VSInput input)
{
    VSOutput output;

    float4 worldPos = mul(model, float4(input.position, 1.0f));

    output.FragPos  = worldPos.xyz;
    output.Normal   = mul(normalMatrix, input.normal);
    output.TexCoords = input.texcoord;

    output.position = mul(projection, mul(view, worldPos));

    return output;
}
