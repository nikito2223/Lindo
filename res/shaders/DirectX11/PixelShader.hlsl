// ============================================================
//  Подключаем адаптированные заголовки
// ============================================================
#include "Lighting.hlsl"
#include "Shadows.hlsl"

// ============================================================
//  Глобальные текстуры материала
// ============================================================
Texture2D gDiffuseTex   : register(t0);
Texture2D gSpecularTex  : register(t1);
SamplerState gSampler   : register(s0);

// ============================================================
//  Основные буферы
// ============================================================
cbuffer CameraData : register(b0)
{
    float3 viewPos;
    float  debugMode;
    float  showLightIcons;
    float  lightIconRadius;
}

cbuffer LightData : register(b1)
{
    int activePointLights;
    int u_pointShadowIndex[NR_POINT_LIGHTS];
}

cbuffer Matrices : register(b2)
{
    float4x4 viewMatrix;
}

cbuffer Lights : register(b3)
{
    DirLight dirLight;
    PointLight pointLights[NR_POINT_LIGHTS];
    SpotLight spotLight;
    Material material;
}

// ============================================================
//  Вход / выход пиксельного шейдера
// ============================================================
struct PSInput
{
    float4 position : SV_POSITION;
    float3 FragPos  : TEXCOORD0;
    float3 Normal   : TEXCOORD1;
    float2 TexCoords: TEXCOORD2;
};

// ============================================================
//  Функции освещения (Lighting.hlsl)
// ============================================================

float3 CalcDirLight(DirLight light, float3 normal, float3 viewDir,
                    float3 diffCol, float3 specCol)
{
    if (!light.enabled) return float3(0,0,0);

    float3 lightDir = normalize(-light.direction);
    float diff = max(dot(normal, lightDir), 0.0);

    float3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0),
                     max(material.shininess, 1.0));

    float3 ambient  = light.ambient * diffCol;
    float3 diffuse  = light.diffuse * diff * diffCol;
    float3 specular = light.specular * spec * specCol * light.color;

    return ambient + diffuse + specular;
}

float3 CalcPointLight(PointLight light, float3 normal, float3 fragPos,
                      float3 viewDir, float3 diffCol, float3 specCol,
                      int shadowIndex)
{
    float3 lightDir = light.position - fragPos;
    float distance = length(lightDir);
    lightDir = normalize(lightDir);

    float denom = light.constant +
                  light.linear * distance +
                  light.quadratic * (distance * distance);

    float attenuation = 1.0 / max(denom, 0.0001);

    if (attenuation < 0.01)
        return float3(0,0,0);

    float diff = max(dot(normal, lightDir), 0.0);
    float3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0),
                     max(material.shininess, 1.0));

    float3 ambient  = light.ambient * diffCol;
    float3 diffuse  = light.diffuse * diff * diffCol;
    float3 specular = light.specular * spec * specCol * light.color;

    float shadow = 0.0;
    if (shadowIndex >= 0)
        shadow = PointShadowCalculation(fragPos, normal, light.position, shadowIndex);

    return (ambient + (1.0 - shadow) * (diffuse + specular)) * attenuation;
}

float3 CalcSpotLight(SpotLight light, float3 normal, float3 fragPos,
                     float3 viewDir, float3 diffCol, float3 specCol)
{
    float3 lightDir = normalize(light.position - fragPos);
    float distance = length(light.position - fragPos);

    float theta = dot(lightDir, normalize(-light.direction));
    float epsilon = max(light.cutOff - light.outerCutOff, 0.0001);
    float spotIntensity = clamp((theta - light.outerCutOff) / epsilon, 0.0, 1.0);

    float diff = max(dot(normal, lightDir), 0.0);
    float3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0),
                     max(material.shininess, 1.0));

    float denom = light.constant +
                  light.linear * distance +
                  light.quadratic * (distance * distance);

    float attenuation = 1.0 / max(denom, 0.0001);

    float3 ambient  = light.ambient * diffCol;
    float3 diffuse  = light.diffuse * diff * diffCol;
    float3 specular = light.specular * spec * specCol * light.color;

    float shadow = SpotShadowCalculation(fragPos, normal, light.position, light.direction);

    float3 lighting = ambient + (1.0 - shadow) * (diffuse + specular);

    return lighting * attenuation * spotIntensity;
}

// ============================================================
//  Основной Pixel Shader
// ============================================================
float4 main(PSInput input) : SV_TARGET
{
    float3 FragPos = input.FragPos;
    float3 Normal  = normalize(input.Normal);

    // === Debug Light Icons ===
    if (debugMode > 0.5 && showLightIcons > 0.5)
    {
        for (int i = 0; i < NR_POINT_LIGHTS; i++)
        {
            if (pointLights[i].enabled)
            {
                float3 toLight = pointLights[i].position - FragPos;
                float dist = length(toLight);

                if (dist < lightIconRadius)
                {
                    float intensity = 1.0 - dist / lightIconRadius;
                    float3 baseColor = pointLights[i].color * pointLights[i].intensity;
                    float alpha = lerp(0.3, 0.9, intensity);
                    return float4(baseColor, alpha);
                }
            }
        }

        if (spotLight.enabled)
        {
            float3 toLight = spotLight.position - FragPos;
            float dist = length(toLight);

            if (dist < lightIconRadius)
            {
                float intensity = 1.0 - dist / lightIconRadius;
                float3 baseColor = spotLight.color * spotLight.intensity;
                float alpha = lerp(0.3, 0.9, intensity);
                return float4(baseColor, alpha);
            }
        }
    }

    // === Diffuse / Specular ===
    float4 diffTex = material.useTexture
        ? gDiffuseTex.Sample(gSampler, input.TexCoords)
        : float4(material.color, 1.0);

    if (diffTex.a < 0.1)
        discard;

    float3 diffCol = diffTex.rgb;
    float3 specCol = material.useTexture
        ? gSpecularTex.Sample(gSampler, input.TexCoords).rgb
        : float3(1,1,1);

    float3 viewDir = normalize(viewPos - FragPos);

    // === Directional Light ===
    float3 result = CalcDirLight(dirLight, Normal, viewDir, diffCol, specCol);

    // === Point Lights ===
    int numLights = min(activePointLights, NR_POINT_LIGHTS);
    for (int i = 0; i < numLights; i++)
    {
        if (pointLights[i].enabled)
        {
            result += CalcPointLight(pointLights[i], Normal, FragPos,
                                     viewDir, diffCol, specCol,
                                     u_pointShadowIndex[i]);
        }
    }

    // === Spot Light ===
    if (spotLight.enabled)
    {
        result += CalcSpotLight(spotLight, Normal, FragPos,
                                viewDir, diffCol, specCol);
    }

    return float4(result, 1.0);
}
