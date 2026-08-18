#version 330 core
out vec4 FragColor;

#include "include/Lighting.glsl"
#include "include/Shadows.glsl"

#define NR_POINT_LIGHTS 32

uniform int activePointLights;
uniform mat4 viewMatrix;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;

uniform vec3 viewPos;
uniform DirLight dirLight;
uniform PointLight pointLights[NR_POINT_LIGHTS];
uniform SpotLight spotLight;
uniform Material material;

uniform int u_pointShadowIndex[NR_POINT_LIGHTS];

// Debug mode
uniform bool debugMode;
uniform bool showLightIcons;
uniform float lightIconRadius;

// Forward Declarations
vec3 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir, vec3 diffCol, vec3 specCol);
vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir, vec3 diffCol, vec3 specCol, int shadowIndex);
vec3 CalcSpotLight(SpotLight light, vec3 normal, vec3 fragPos, vec3 viewDir, vec3 diffCol, vec3 specCol);

void main()
{
    if (debugMode && showLightIcons) {
        for (int i = 0; i < NR_POINT_LIGHTS; i++) {
            if (pointLights[i].enabled) {
                vec3 toLight = pointLights[i].position - FragPos;
                float distToLight = length(toLight);
                if (distToLight < lightIconRadius) {
                    float intensity = 1.0 - distToLight / lightIconRadius;
                    vec3 baseColor = pointLights[i].color * pointLights[i].intensity;
                    float alpha = mix(0.3, 0.9, intensity);
                    FragColor = vec4(baseColor, alpha);
                    return;
                }
            }
        }
        if (spotLight.enabled) {
            vec3 toLight = spotLight.position - FragPos;
            float distToLight = length(toLight);
            if (distToLight < lightIconRadius) {
                float intensity = 1.0 - distToLight / lightIconRadius;
                vec3 baseColor = spotLight.color * spotLight.intensity;
                float alpha = mix(0.3, 0.9, intensity);
                FragColor = vec4(baseColor, alpha);
                return;
            }
        }
    }

    vec4 diffTex = material.useTexture ? texture(material.diffuse, TexCoords) : vec4(material.color, 1.0);
    if (diffTex.a < 0.1) discard;

    vec3 diffCol = diffTex.rgb;
    vec3 specCol = material.useTexture ? texture(material.specular, TexCoords).rgb : vec3(1.0);

    vec3 norm = normalize(Normal);
    vec3 viewDir = length(viewPos - FragPos) > 0.0001 ? normalize(viewPos - FragPos) : vec3(0.0, 0.0, 1.0);

    vec3 result = CalcDirLight(dirLight, norm, viewDir, diffCol, specCol);

    int numLights = min(activePointLights, NR_POINT_LIGHTS);
    for (int i = 0; i < numLights; i++) {
        if (pointLights[i].enabled) {
            result += CalcPointLight(pointLights[i], norm, FragPos, viewDir, diffCol, specCol, u_pointShadowIndex[i]);
        }
    }

    if (spotLight.enabled) {
        result += CalcSpotLight(spotLight, norm, FragPos, viewDir, diffCol, specCol);
    }

    FragColor = vec4(result, 1.0);
}

vec3 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir, vec3 diffCol, vec3 specCol)
{
    if (!light.enabled) return vec3(0.0);

    vec3 lightDir = normalize(-light.direction);
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), max(material.shininess, 1.0));

    vec3 ambient  = light.ambient * diffCol;
    vec3 diffuse  = light.diffuse * diff * diffCol;
    vec3 specular = light.specular * spec * specCol * light.color;

    if (u_shadowsEnabled) {
        vec4 viewPos4 = viewMatrix * vec4(FragPos, 1.0);
        float viewDepth = -viewPos4.z;
        
        float shadow = CascadedShadowCalculation(FragPos, normal, lightDir, viewDepth);
        diffuse *= (1.0 - shadow);
        specular *= (1.0 - shadow);
    }

    return ambient + diffuse + specular;
}

vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir, vec3 diffCol, vec3 specCol, int shadowIndex)
{
    vec3 lightDir = light.position - fragPos;
    float distance = length(lightDir);
    lightDir = normalize(lightDir);

    float denom = light.constant + light.linear * distance + light.quadratic * (distance * distance);
    float attenuation = 1.0 / max(denom, 0.0001);

    // ОПТИМИЗАЦИЯ: Если свет от этого источника уже не виден (менее 1%), 
    // мы даже не считаем для него блики и тяжелые тени
    if (attenuation < 0.01) {
        return vec3(0.0);
    }

    float diff = max(dot(normal, lightDir), 0.0);
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), max(material.shininess, 1.0));

    vec3 ambient  = light.ambient * diffCol;
    vec3 diffuse  = light.diffuse * diff * diffCol;
    vec3 specular = light.specular * spec * specCol * light.color;

    // Считаем тень только если источник реально бросает тень
    float shadow = 0.0;
    if (shadowIndex >= 0 && u_shadowsEnabled) {
        shadow = PointShadowCalculation(fragPos, normal, light.position, shadowIndex);
    }

    return (ambient + (1.0 - shadow) * (diffuse + specular)) * attenuation;
}

vec3 CalcSpotLight(SpotLight light, vec3 normal, vec3 fragPos, vec3 viewDir, vec3 diffCol, vec3 specCol)
{
    vec3 lightDir = normalize(light.position - fragPos);
    float distance = length(light.position - fragPos);

    float theta = dot(lightDir, normalize(-light.direction));
    float epsilon = max(light.cutOff - light.outerCutOff, 0.0001);
    float spotIntensity = clamp((theta - light.outerCutOff) / epsilon, 0.0, 1.0);

    float diff = max(dot(normal, lightDir), 0.0);
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), max(material.shininess, 1.0));

    float denom = light.constant + light.linear * distance + light.quadratic * (distance * distance);
    float attenuation = 1.0 / max(denom, 0.0001);

    vec3 ambient  = light.ambient * diffCol;
    vec3 diffuse  = light.diffuse * diff * diffCol;
    vec3 specular = light.specular * spec * specCol * light.color;

    float shadow = SpotShadowCalculation(fragPos, normal, light.position, light.direction);
    vec3 lighting = ambient + (1.0 - shadow) * (diffuse + specular);

    return lighting * attenuation * spotIntensity;
}