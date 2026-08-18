#ifndef LIGHTING_HLSL
#define LIGHTING_HLSL

// Текстуры и сэмплеры — отдельные ресурсы
Texture2D gDiffuseTex   : register(t0);
Texture2D gSpecularTex  : register(t1);
SamplerState gSampler   : register(s0);

struct Material
{
    float  shininess;
    int    useTexture;   // bool → int/uint
    float3 color;
};

struct DirLight
{
    float3 direction;
    float3 color;
    float  intensity;
    float3 ambient;
    float3 diffuse;
    float3 specular;
    int    enabled;
};

struct PointLight
{
    float3 position;
    float3 color;
    float  intensity;
    float  constant;
    float  linear;
    float  quadratic;
    float  radius;
    float3 ambient;
    float3 diffuse;
    float3 specular;
    int    enabled;
};

struct SpotLight
{
    float3 position;
    float3 direction;
    float  cutOff;
    float  outerCutOff;
    float3 color;
    float  intensity;
    float  constant;
    float  linear;
    float  quadratic;
    float3 ambient;
    float3 diffuse;
    float3 specular;
    int    enabled;
};

#endif // LIGHTING_HLSL
