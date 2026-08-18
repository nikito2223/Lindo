#ifndef SHADOWS_GLSL
#define SHADOWS_GLSL

#define MAX_CASCADES 4

// === CSM Uniforms ===
uniform sampler2DArray u_shadowMap;
uniform bool u_shadowsEnabled;
uniform int u_cascadeCount;
uniform float u_cascadeSplitPlanes[MAX_CASCADES];
uniform mat4 u_shadowMatrices[MAX_CASCADES];
uniform float u_shadowMapSize;
uniform float u_penumbraScale;
uniform float u_cascadeBlend;

// === Point Shadows Uniforms ===
uniform samplerCube u_pointShadowMaps[4];
uniform float u_pointShadowFarPlanes[4];
uniform vec3 u_pointShadowPositions[4];
uniform int u_pointShadowCount;

// === Spot Shadows Uniforms ===
uniform sampler2D u_spotShadowMap;
uniform bool u_spotShadowsEnabled;
uniform mat4 u_spotShadowMatrix;
uniform vec3 u_spotLightPosition;
uniform vec3 u_spotLightDirection;
uniform float u_spotShadowFarPlane;

const vec3 sampleOffsetDirections[20] = vec3[](
    vec3( 1,  1,  1), vec3( 1, -1,  1), vec3(-1, -1,  1), vec3(-1,  1,  1),
    vec3( 1,  1, -1), vec3( 1, -1, -1), vec3(-1, -1, -1), vec3(-1,  1, -1),
    vec3( 1,  1,  0), vec3( 1, -1,  0), vec3(-1, -1,  0), vec3(-1,  1,  0),
    vec3( 1,  0,  1), vec3(-1,  0,  1), vec3( 1,  0, -1), vec3(-1,  0, -1),
    vec3( 0,  1,  1), vec3( 0, -1,  1), vec3( 0,  1, -1), vec3( 0, -1, -1)
);

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

float PCFDirectional(vec2 projCoords, int cascade, float currentDepth, float bias)
{
    vec2 texelSize = vec2(1.0) / vec2(textureSize(u_shadowMap, 0).xy);
    float shadow = 0.0;
    int kernel = 2;
    for (int x = -kernel; x <= kernel; ++x) {
        for (int y = -kernel; y <= kernel; ++y) {
            float pcfDepth = texture(u_shadowMap, vec3(projCoords.xy + vec2(x, y) * texelSize, float(cascade))).r;
            shadow += (currentDepth - bias) > pcfDepth ? 1.0 : 0.0;
        }
    }
    return shadow / 25.0;
}

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
    int kernel = 3;
    for (int x = -kernel; x <= kernel; ++x) {
        for (int y = -kernel; y <= kernel; ++y) {
            vec2 offset = vec2(x, y) * texelSize * filterRadius;
            float pcfDepth = texture(u_shadowMap, vec3(projCoords.xy + offset, float(cascade))).r;
            shadow += (currentDepth - bias) > pcfDepth ? 1.0 : 0.0;
        }
    }
    return shadow / 49.0;
}

float CascadedShadowCalculation(vec3 fragPos, vec3 normal, vec3 lightDir, float viewDepth)
{
    if (!u_shadowsEnabled) return 0.0;

    int cascade = selectCascade(viewDepth);
    float normalBiasScale = 0.02;
    vec3 offsetFragPos = fragPos + normal * normalBiasScale;

    vec4 fragPosLightSpace = u_shadowMatrices[cascade] * vec4(offsetFragPos, 1.0);
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;

    if (projCoords.z > 1.0 || projCoords.z < 0.0 ||
        projCoords.x < 0.0 || projCoords.x > 1.0 ||
        projCoords.y < 0.0 || projCoords.y > 1.0) {
        return 0.0;
    }

    float currentDepth = projCoords.z;
    float bias = max(0.0005, 0.003 * (1.0 - dot(normal, lightDir)));

    float shadow;
    if (u_penumbraScale > 0.0) {
        shadow = PCSSDirectional(projCoords.xy, cascade, normal, lightDir, currentDepth, bias);
    } else {
        shadow = PCFDirectional(projCoords.xy, cascade, currentDepth, bias);
    }

    float blend = u_cascadeBlend;
    if (cascade > 0 && viewDepth > u_cascadeSplitPlanes[cascade - 1]) {
        float t = clamp((viewDepth - u_cascadeSplitPlanes[cascade - 1]) / blend, 0.0, 1.0);
        shadow *= (1.0 - t);
    }

    return clamp(shadow, 0.0, 1.0);
}

float PointShadowCalculation(vec3 fragPos, vec3 normal, vec3 lightPos, int shadowIndex)
{
    if (shadowIndex < 0 || shadowIndex >= u_pointShadowCount) return 0.0;

    vec3 fragToLight = fragPos - lightPos;
    float currentDepth = length(fragToLight);
    float farPlane = u_pointShadowFarPlanes[shadowIndex];

    // ќтсекаем пиксели, которые наход€тс€ дальше границы тени источника
    if (currentDepth > farPlane) return 0.0;

    vec3 lightDir = normalize(lightPos - fragPos);
    float bias = max(0.05 * (1.0 - dot(normal, lightDir)), 0.005);

    float shadow = 0.0;
    float diskRadius = 0.05;
    
    // ÷икл теперь использует предкомпилированный глобальный массив
    for (int i = 0; i < 20; ++i) {
        float closest = texture(u_pointShadowMaps[shadowIndex], fragToLight + sampleOffsetDirections[i] * diskRadius).r;
        closest *= farPlane;
        if (currentDepth - bias > closest) shadow += 1.0;
    }
    shadow /= 20.0;

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

#endif // SHADOWS_GLSL