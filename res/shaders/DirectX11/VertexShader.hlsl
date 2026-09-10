cbuffer TransformBuffer : register(b0)
{
    float4x4 model;
    float4x4 view;
    float4x4 projection;
};

struct VSInput
{
    float3 position : POSITION;
    float3 normal : NORMAL;
    float2 texCoord : TEXCOORD0;
};

struct VSOutput
{
    float4 position : SV_POSITION;
    float3 worldPosition : TEXCOORD1;
    float3 normal : NORMAL;
    float2 texCoord : TEXCOORD0;
};

VSOutput main(VSInput input)
{
    VSOutput output;
    float4 worldPosition = mul(float4(input.position, 1.0f), model);

    output.position = mul(mul(worldPosition, view), projection);
    output.worldPosition = worldPosition.xyz;
    output.normal = normalize(mul(input.normal, (float3x3)model));
    output.texCoord = input.texCoord;
    return output;
}
