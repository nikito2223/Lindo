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

// --- Post / tonemapping ---
uniform float exposure = 1.0f;   // default'ы в GLSL 330 можно
uniform float gamma    = 2.2f;   // но лучше без них и просто объявить

// Forward Declarations
vec3 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir, vec3 diffCol, vec3 specCol);
vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir, vec3 diffCol, vec3 specCol);
vec3 CalcSpotLight(SpotLight light, vec3 normal, vec3 fragPos, vec3 viewDir, vec3 diffCol, vec3 specCol);

void main()
{
    // --- UV-трансформы (Unity-стиль): uv * tiling + offset ---
    vec2 diffuseUV  = TexCoords * material.diffuseTiling  + material.diffuseOffset;
    vec2 specularUV = TexCoords * material.specularTiling + material.specularOffset;

    // diffuse: если есть текстура — берём её и тонируем material.color,
    // иначе — плоский material.color.
    vec3 diffCol;
    if (material.useTexture) {
        vec4 texel = texture(material.diffuse, diffuseUV);
        if (texel.a < 0.1) discard;
        diffCol = texel.rgb * material.color;
    } else {
        diffCol = material.color;
    }

    // specular: сэмплим по своему UV (может иметь другой tiling/offset)
    vec3 specCol = material.useTexture
        ? texture(material.specular, specularUV).rgb
        : vec3(1.0);

    vec3 norm    = normalize(Normal);
    vec3 viewDir = length(viewPos - FragPos) > 0.0001
        ? normalize(viewPos - FragPos)
        : vec3(0.0, 0.0, 1.0);

    vec3 result = diffCol * 0.03
                + CalcDirLight(dirLight, norm, viewDir, diffCol, specCol);

    int numLights = min(activePointLights, NR_POINT_LIGHTS);
    for (int i = 0; i < numLights; i++) {
        if (pointLights[i].enabled) {
            result += CalcPointLight(pointLights[i], norm, FragPos, viewDir, diffCol, specCol);
        }
    }

    if (spotLight.enabled) {
        result += CalcSpotLight(spotLight, norm, FragPos, viewDir, diffCol, specCol);
    }

    vec3 color = result * exposure;
    color = color / (color + vec3(1.0));       // опционально Reinhard
    color = pow(color, vec3(1.0 / gamma));
    FragColor = vec4(color, 1.0);
}

vec3 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir, vec3 diffCol, vec3 specCol)
{
    if (!light.enabled) return vec3(0.0);

    vec3 lightDir = normalize(-light.direction);
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), max(material.shininess, 1.0));

    vec3 ambient  = light.ambient  * light.color * diffCol;
    vec3 diffuse  = light.diffuse  * light.color * diff  * diffCol;
    vec3 specular = light.specular * light.color * spec * specCol;

    if (u_shadowsEnabled) {
        vec4 viewPosition = viewMatrix * vec4(FragPos, 1.0);
        float shadow = CascadedShadowCalculation(FragPos, normal, lightDir, -viewPosition.z);
        diffuse *= 1.0 - shadow;
        specular *= 1.0 - shadow;
    }

    return ambient + diffuse + specular;
}

vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir, vec3 diffCol, vec3 specCol)
{
    vec3 lightDir = light.position - fragPos;
    float distance = length(lightDir);
    lightDir = normalize(lightDir);

    float denom = light.constant + light.linear * distance + light.quadratic * (distance * distance);
    float attenuation = 1.0 / max(denom, 0.0001);

    if (attenuation < 0.01) {
        return vec3(0.0);
    }

    float diff = max(dot(normal, lightDir), 0.0);
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), max(material.shininess, 1.0));

    vec3 ambient  = light.ambient  * light.color * diffCol;
    vec3 diffuse  = light.diffuse  * light.color * diff  * diffCol;
    vec3 specular = light.specular * light.color * spec * specCol;

    return (ambient + diffuse + specular) * attenuation;
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

    vec3 ambient  = light.ambient  * light.color * diffCol;
    vec3 diffuse  = light.diffuse  * light.color * diff  * diffCol;
    vec3 specular = light.specular * light.color * spec * specCol;

    float shadow = SpotShadowCalculation(fragPos, normal, light.position, light.direction);

    vec3 lighting = ambient + (1.0 - shadow) * (diffuse + specular);

    return lighting * attenuation * spotIntensity;
}