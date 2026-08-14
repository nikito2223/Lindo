#version 330 core
out vec4 FragColor;

struct Material {
    sampler2D diffuse;
    sampler2D specular;
    float shininess;
    bool useTexture;
    vec3 color;
};

struct DirLight {
    vec3 direction;
    vec3 color;
    float intensity;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    bool enabled;
};

struct PointLight {
    vec3 position;
    vec3 color;
    float intensity;
    float constant;
    float linear;
    float quadratic;
    float radius;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    bool enabled;
};

struct SpotLight {
    vec3 position;
    vec3 direction;
    float cutOff;
    float outerCutOff;
    vec3 color;
    float intensity;
    float constant;
    float linear;
    float quadratic;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    bool enabled;
};

#define NR_POINT_LIGHTS 32
#define MAX_CASCADES 4
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

// === CSM (Cascaded Shadow Maps) ===
uniform sampler2DArray u_shadowMap;       // 2D array, one layer per cascade
uniform bool u_shadowsEnabled;
uniform int u_cascadeCount;
uniform float u_cascadeSplitPlanes[MAX_CASCADES];
uniform mat4 u_shadowMatrices[MAX_CASCADES];
uniform float u_shadowMapSize;            // directional map resolution
uniform float u_penumbraScale;            // PCSS blocker scale
uniform float u_cascadeBlend;

// === Point shadows (cubemaps) ===
uniform samplerCube u_pointShadowMaps[4];
uniform float u_pointShadowFarPlanes[4];
uniform vec3 u_pointShadowPositions[4];
uniform int u_pointShadowCount;
uniform int u_pointShadowIndex[NR_POINT_LIGHTS];

uniform sampler2D u_spotShadowMap;
uniform bool u_spotShadowsEnabled;
uniform mat4 u_spotShadowMatrix;
uniform vec3 u_spotLightPosition;
uniform vec3 u_spotLightDirection;
uniform float u_spotShadowFarPlane;

// Debug mode
uniform bool debugMode;
uniform bool showLightIcons;
uniform float lightIconRadius;

// Prototypes
vec3 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir, vec3 diffCol, vec3 specCol);
vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir, vec3 diffCol, vec3 specCol, int shadowIndex);
vec3 CalcSpotLight(SpotLight light, vec3 normal, vec3 fragPos, vec3 viewDir, vec3 diffCol, vec3 specCol);

int selectCascade(float viewDepth);
float CascadedShadowCalculation(vec3 fragPos, vec3 normal, vec3 lightDir, float viewDepth);

float PointShadowCalculation(vec3 fragPos, vec3 normal, vec3 lightPos, int shadowIndex);
float SpotShadowCalculation(vec3 fragPos, vec3 normal, vec3 lightPos, vec3 lightDir);
float PCSSDirectional(vec2 projCoords, int cascade, vec3 normal, vec3 lightDir, float currentDepth, float bias);
float BlockerSearch(vec2 projCoords, int cascade, float currentDepth, float bias, float searchRadius);
float PCFDirectional(vec2 projCoords, int cascade, float currentDepth, float bias);

void main()
{
    // ==== Рендер иконок источников света ====
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

    // 1. Предварительный расчет цветов
    vec4 diffTex = material.useTexture ? texture(material.diffuse, TexCoords) : vec4(material.color, 1.0);
    if (diffTex.a < 0.1) discard;

    vec3 diffCol = diffTex.rgb;
    vec3 specCol = material.useTexture ? texture(material.specular, TexCoords).rgb : vec3(1.0);

    vec3 norm = normalize(Normal);
    vec3 viewDir = length(viewPos - FragPos) > 0.0001 ? normalize(viewPos - FragPos) : vec3(0.0, 0.0, 1.0);

    // Фаза 1: Направленный свет (Солнце) с CSM
    vec3 result = CalcDirLight(dirLight, norm, viewDir, diffCol, specCol);

    // Фаза 2: Точечные источники
    int numLights = min(activePointLights, NR_POINT_LIGHTS);
    for (int i = 0; i < numLights; i++) {
        if (pointLights[i].enabled) {
            result += CalcPointLight(pointLights[i], norm, FragPos, viewDir, diffCol, specCol, u_pointShadowIndex[i]);
        }
    }

    // Фаза 3: Прожектор
    if (spotLight.enabled) {
        result += CalcSpotLight(spotLight, norm, FragPos, viewDir, diffCol, specCol);
    }

    FragColor = vec4(result, 1.0);
}

// === ВЫБОР КАСКАДА (исправлено получение параметра depths) ===
int selectCascade(float viewDepth)
{
    int cascade = 0;
    for (int i = 0; i < u_cascadeCount - 1; ++i) {
        if (viewDepth > u_cascadeSplitPlanes[i]) {
            cascade = i + 1;
        }
    }
    return clamp(cascade, 0, u_cascadeCount - 1);
}

// === PCF ДЛЯ CSM (2D array) ===
float PCFDirectional(vec2 projCoords, int cascade, float currentDepth, float bias)
{
    vec2 texelSize = vec2(1.0) / vec2(textureSize(u_shadowMap, 0).xy);
    float shadow = 0.0;
    int kernel = 2; // 2 => 5x5 taps
    for (int x = -kernel; x <= kernel; ++x) {
        for (int y = -kernel; y <= kernel; ++y) {
            float pcfDepth = texture(u_shadowMap, vec3(projCoords.xy + vec2(x, y) * texelSize, float(cascade))).r;
            shadow += (currentDepth - bias) > pcfDepth ? 1.0 : 0.0;
        }
    }
    return shadow / 25.0;
}

// === ПОИСК БЛОКЕРА (PCSS) ===
float BlockerSearch(vec2 projCoords, int cascade, float currentDepth, float bias, float searchRadius)
{
    vec2 texelSize = vec2(1.0) / vec2(textureSize(u_shadowMap, 0).xy);
    float blockerDepth = 0.0;
    int count = 0;
    for (int x = -2; x <= 2; ++x) {
        for (int y = -2; y <= 2; ++y) {
            float pcfDepth = texture(u_shadowMap, vec3(projCoords.xy + vec2(x, y) * texelSize * searchRadius, float(cascade))).r;
            if ((currentDepth - bias) > pcfDepth) {
                blockerDepth += pcfDepth;
                count++;
            }
        }
    }
    if (count == 0) return -1.0;
    return blockerDepth / float(count);
}

// === PCSS ДЛЯ CSM ===
float PCSSDirectional(vec2 projCoords, int cascade, vec3 normal, vec3 lightDir, float currentDepth, float bias)
{
    vec2 texelSize = vec2(1.0) / vec2(textureSize(u_shadowMap, 0).xy);
    float scale = u_penumbraScale;
    float searchRadius = 2.0 * scale;

    float avgBlockerDepth = BlockerSearch(projCoords, cascade, currentDepth, bias, searchRadius);
    if (avgBlockerDepth < 0.0) return 0.0;

    float penumbra = (currentDepth - avgBlockerDepth) / avgBlockerDepth;
    float filterRadius = clamp(penumbra * scale * 3.0, 0.0, 8.0);

    float shadow = 0.0;
    int kernel = 3; // 3 => 7x7 taps
    for (int x = -kernel; x <= kernel; ++x) {
        for (int y = -kernel; y <= kernel; ++y) {
            vec2 offset = vec2(x, y) * texelSize * filterRadius;
            float pcfDepth = texture(u_shadowMap, vec3(projCoords.xy + offset, float(cascade))).r;
            shadow += (currentDepth - bias) > pcfDepth ? 1.0 : 0.0;
        }
    }
    return shadow / 49.0;
}

// === CSM РАСЧЕТ ТЕНИ (исправлен вызов selectCascade) ===
float CascadedShadowCalculation(vec3 fragPos, vec3 normal, vec3 lightDir, float viewDepth)
{
    if (!u_shadowsEnabled) return 0.0;

    // Используем переданную viewDepth вместо вычисления
    int cascade = selectCascade(viewDepth);

    vec4 fragPosLightSpace = u_shadowMatrices[cascade] * vec4(fragPos, 1.0);
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;

    if (projCoords.z > 1.0 || projCoords.z < 0.0 ||
        projCoords.x < 0.0 || projCoords.x > 1.0 ||
        projCoords.y < 0.0 || projCoords.y > 1.0) {
        return 0.0;
    }

    float currentDepth = projCoords.z;
    float bias = max(0.005, 0.02 * (1.0 - dot(normal, lightDir)));

    vec2 texelSize = vec2(1.0) / vec2(textureSize(u_shadowMap, 0).xy);
    vec2 offset = normal.xy * texelSize * 2.0;
    vec3 offsetCoords = vec3(projCoords.xy + offset, projCoords.z);
    currentDepth = offsetCoords.z;

    float shadow;
    if (u_penumbraScale > 0.0) {
        shadow = PCSSDirectional(offsetCoords.xy, cascade, normal, lightDir, currentDepth, bias);
    } else {
        shadow = PCFDirectional(offsetCoords.xy, cascade, currentDepth, bias);
    }

    float blend = u_cascadeBlend;
    if (cascade > 0 && viewDepth > u_cascadeSplitPlanes[cascade - 1]) {
        float t = clamp((viewDepth - u_cascadeSplitPlanes[cascade - 1]) / blend, 0.0, 1.0);
        shadow *= (1.0 - t);
    }

    return clamp(shadow, 0.0, 1.0);
}

// === РАСЧЕТ ТЕНИ ОТ ТОЧЕЧНОГО СВЕТА (CUBE) ===
float PointShadowCalculation(vec3 fragPos, vec3 normal, vec3 lightPos, int shadowIndex)
{
    if (shadowIndex < 0 || shadowIndex >= u_pointShadowCount) return 0.0;

    vec3 fragToLight = fragPos - lightPos;
    float currentDepth = length(fragToLight);
    float farPlane = u_pointShadowFarPlanes[shadowIndex];
    float closestDepth = texture(u_pointShadowMaps[shadowIndex], fragToLight).r;
    closestDepth *= farPlane;

    vec3 lightDir = normalize(lightPos - fragPos);
    float bias = max(0.05 * (1.0 - dot(normal, lightDir)), 0.005);

    float shadow = 0.0;
    vec3 sampleOffsetDirections[20] = vec3[](
        vec3( 1,  1,  1), vec3( 1, -1,  1), vec3(-1, -1,  1), vec3(-1,  1,  1),
        vec3( 1,  1, -1), vec3( 1, -1, -1), vec3(-1, -1, -1), vec3(-1,  1, -1),
        vec3( 1,  1,  0), vec3( 1, -1,  0), vec3(-1, -1,  0), vec3(-1,  1,  0),
        vec3( 1,  0,  1), vec3(-1,  0,  1), vec3( 1,  0, -1), vec3(-1,  0, -1),
        vec3( 0,  1,  1), vec3( 0, -1,  1), vec3( 0,  1, -1), vec3( 0, -1, -1)
    );

    float diskRadius = 0.05;
    for (int i = 0; i < 20; ++i) {
        float closest = texture(u_pointShadowMaps[shadowIndex], fragToLight + sampleOffsetDirections[i] * diskRadius).r;
        closest *= farPlane;
        if (currentDepth - bias > closest) shadow += 1.0;
    }
    shadow /= 20.0;

    if (currentDepth > farPlane) shadow = 0.0;
    return clamp(shadow, 0.0, 1.0);
}

float SpotShadowCalculation(vec3 fragPos, vec3 normal, vec3 lightPos, vec3 lightDir)
{
    if (!u_spotShadowsEnabled) return 0.0;

    vec4 fragPosLightSpace = u_spotShadowMatrix * vec4(fragPos, 1.0);
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;

    if (projCoords.z > 1.0 || projCoords.z < 0.0 ||
        projCoords.x < 0.0 || projCoords.x > 1.0 ||
        projCoords.y < 0.0 || projCoords.y > 1.0) {
        return 0.0;
    }

    float currentDepth = projCoords.z;
    float bias = max(0.005, 0.02 * (1.0 - dot(normal, lightDir)));
    vec2 texelSize = vec2(1.0) / vec2(textureSize(u_spotShadowMap, 0).xy);
    float shadow = 0.0;
    int kernel = 1;
    for (int x = -kernel; x <= kernel; ++x) {
        for (int y = -kernel; y <= kernel; ++y) {
            float pcfDepth = texture(u_spotShadowMap, projCoords.xy + vec2(x, y) * texelSize).r;
            shadow += (currentDepth - bias) > pcfDepth ? 1.0 : 0.0;
        }
    }
    shadow /= float((2 * kernel + 1) * (2 * kernel + 1));

    if (length(fragPos - lightPos) > u_spotShadowFarPlane) shadow = 0.0;
    return clamp(shadow, 0.0, 1.0);
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
        // Вычисляем глубину в пространстве вида
        vec4 viewPos4 = viewMatrix * vec4(FragPos, 1.0);
        float viewDepth = -viewPos4.z; // Глубина в пространстве вида
        
        float shadow = CascadedShadowCalculation(FragPos, normal, lightDir, viewDepth);
        diffuse *= (1.0 - shadow);
        specular *= (1.0 - shadow);
    }

    return ambient + diffuse + specular;
}

vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir, vec3 diffCol, vec3 specCol, int shadowIndex)
{
    vec3 lightDir = normalize(light.position - fragPos);
    float distance = length(light.position - fragPos);

    float diff = max(dot(normal, lightDir), 0.0);
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), max(material.shininess, 1.0));

    float denom = light.constant + light.linear * distance + light.quadratic * (distance * distance);
    float attenuation = 1.0 / max(denom, 0.0001);

    vec3 ambient  = light.ambient * diffCol;
    vec3 diffuse  = light.diffuse * diff * diffCol;
    vec3 specular = light.specular * spec * specCol * light.color;

    float shadow = PointShadowCalculation(fragPos, normal, light.position, shadowIndex);

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