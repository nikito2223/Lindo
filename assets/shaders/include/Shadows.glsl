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
uniform float u_minDepthBias;
uniform float u_maxDepthBias;
uniform float u_normalBias;
uniform float u_texelSpacing;
uniform int u_pcfKernel;

uniform samplerCube u_pointShadowMaps[4];
uniform int u_pointShadowCount;
uniform float u_pointShadowFarPlanes[4];
uniform vec3 u_pointShadowPositions[4];
uniform int u_pointShadowIndex[32];

// === Spot Shadows Uniforms ===
uniform sampler2D u_spotShadowMap;
uniform bool u_spotShadowsEnabled;
uniform mat4 u_spotShadowMatrix;
uniform vec3 u_spotLightPosition;
uniform vec3 u_spotLightDirection;
uniform float u_spotShadowFarPlane;

// Проверка каскада от ближнего сплита к дальнему
int selectCascade(float viewDepth)
{
    int cascade = u_cascadeCount - 1;
    for (int i = 0; i < u_cascadeCount; ++i) {
        if (viewDepth < u_cascadeSplitPlanes[i]) {
            cascade = i;
            break;
        }
    }
    return clamp(cascade, 0, u_cascadeCount - 1);
}

// Аппаратный 3x3 PCF с фильтрацией (дает 4x4 мягкий переход на GPU)
float PCFDirectional(vec2 projCoords, int cascade, float currentDepth, float bias)
{
    ivec2 mapSize = textureSize(u_shadowMap, 0).xy;
    vec2 texelSize = 1.0 / vec2(mapSize);
    int radius = clamp((u_pcfKernel - 1) / 2, 1, 2);
    float shadow = 0.0;
    float count = 0.0;
    for (int x = -2; x <= 2; ++x) {
        for (int y = -2; y <= 2; ++y) {
            if (abs(x) > radius || abs(y) > radius) continue;
            vec2 uv = projCoords + vec2(x, y) * texelSize * max(u_texelSpacing, 0.5);
            if (any(lessThan(uv, vec2(0.0))) || any(greaterThanEqual(uv, vec2(1.0)))) {
                count += 1.0;
                continue;
            }
            vec2 samplePosition = uv * vec2(mapSize) - 0.5;
            ivec2 basePixel = ivec2(floor(samplePosition));
            vec2 sampleBlend = fract(samplePosition);
            ivec2 pixel00 = clamp(basePixel, ivec2(0), mapSize - 1);
            ivec2 pixel10 = clamp(basePixel + ivec2(1, 0), ivec2(0), mapSize - 1);
            ivec2 pixel01 = clamp(basePixel + ivec2(0, 1), ivec2(0), mapSize - 1);
            ivec2 pixel11 = clamp(basePixel + ivec2(1, 1), ivec2(0), mapSize - 1);
            float compare00 = currentDepth - bias > texelFetch(u_shadowMap, ivec3(pixel00, cascade), 0).r ? 1.0 : 0.0;
            float compare10 = currentDepth - bias > texelFetch(u_shadowMap, ivec3(pixel10, cascade), 0).r ? 1.0 : 0.0;
            float compare01 = currentDepth - bias > texelFetch(u_shadowMap, ivec3(pixel01, cascade), 0).r ? 1.0 : 0.0;
            float compare11 = currentDepth - bias > texelFetch(u_shadowMap, ivec3(pixel11, cascade), 0).r ? 1.0 : 0.0;
            float compareTop = mix(compare00, compare10, sampleBlend.x);
            float compareBottom = mix(compare01, compare11, sampleBlend.x);
            shadow += mix(compareTop, compareBottom, sampleBlend.y);
            count += 1.0;
        }
    }
    return shadow / max(count, 1.0);
}

// Поиск средних блокеров для PCSS
float BlockerSearch(vec2 projCoords, int cascade, float currentDepth, float bias, float searchRadius)
{
    ivec2 mapSize = textureSize(u_shadowMap, 0).xy;
    vec2 texelSize = 1.0 / vec2(mapSize);
    float blockerDepth = 0.0;
    int count = 0;
    
    for (int x = -2; x <= 2; ++x) {
        for (int y = -2; y <= 2; ++y) {
            vec2 offset = vec2(x, y) * texelSize * searchRadius;
            vec2 uv = projCoords + offset;
            if (any(lessThan(uv, vec2(0.0))) || any(greaterThanEqual(uv, vec2(1.0)))) continue;
            ivec2 pixel = ivec2(uv * vec2(mapSize));
            float storedDepth = texelFetch(u_shadowMap, ivec3(pixel, cascade), 0).r;
            if (storedDepth < currentDepth - bias) {
                blockerDepth += storedDepth;
                count++;
            }
        }
    }
    if (count == 0) return -1.0;
    return blockerDepth / float(count);
}

// Программно-аппаратный PCSS
float PCSSDirectional(vec2 projCoords, int cascade, vec3 normal, vec3 lightDir, float currentDepth, float bias)
{
    vec2 texelSize = vec2(1.0) / vec2(textureSize(u_shadowMap, 0).xy);
    float scale = u_penumbraScale;
    float searchRadius = 2.0 * scale;

    float avgBlockerDepth = BlockerSearch(projCoords, cascade, currentDepth, bias, searchRadius);
    if (avgBlockerDepth < 0.0) return 0.0;

    float penumbra = max(currentDepth - avgBlockerDepth, 0.0) / max(avgBlockerDepth, 0.0001);
    float filterRadius = clamp(penumbra * scale * 3.0, 1.0, 6.0);

    ivec2 mapSize = textureSize(u_shadowMap, 0).xy;
    float shadow = 0.0;
    float count = 0.0;

    for (int x = -2; x <= 2; ++x) {
        for (int y = -2; y <= 2; ++y) {
            vec2 uv = projCoords.xy + vec2(x, y) * texelSize * filterRadius;
            if (any(lessThan(uv, vec2(0.0))) || any(greaterThanEqual(uv, vec2(1.0)))) {
                count += 1.0;
                continue;
            }
            ivec2 pixel = clamp(ivec2(uv * vec2(mapSize)), ivec2(0), mapSize - 1);
            float storedDepth = texelFetch(u_shadowMap, ivec3(pixel, cascade), 0).r;
            shadow += currentDepth - bias > storedDepth ? 1.0 : 0.0;
            count += 1.0;
        }
    }
    return shadow / count;
}

float CascadedShadowCalculation(vec3 fragPos, vec3 normal, vec3 lightDir, float viewDepth)
{
    if (!u_shadowsEnabled) return 0.0;

    int cascade = selectCascade(viewDepth);
    
    float normalLightAngle = 1.0 - max(dot(normal, lightDir), 0.0);
    vec3 offsetFragPos = fragPos + normal * (u_normalBias * normalLightAngle);

    vec4 fragPosLightSpace = u_shadowMatrices[cascade] * vec4(offsetFragPos, 1.0);
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;

    // Проверка попадания за границы фрустума света
    if (projCoords.z > 1.0 || projCoords.z < 0.0 ||
        projCoords.x < 0.0 || projCoords.x > 1.0 ||
        projCoords.y < 0.0 || projCoords.y > 1.0) {
        return 0.0;
    }

    float currentDepth = projCoords.z;
    
    float bias = max(u_minDepthBias, u_maxDepthBias * normalLightAngle);
    bias *= 1.0 + 0.15 * float(cascade);

    float shadow;
    if (u_penumbraScale > 0.0) {
        shadow = PCSSDirectional(projCoords.xy, cascade, normal, lightDir, currentDepth, bias);
    } else {
        shadow = PCFDirectional(projCoords.xy, cascade, currentDepth, bias);
    }

    // Плавное смешивание каскадов на границах
    if (cascade < u_cascadeCount - 1 && u_cascadeBlend > 0.0) {
        float nextSplit = u_cascadeSplitPlanes[cascade];
        float previousSplit = cascade > 0 ? u_cascadeSplitPlanes[cascade - 1] : 0.0;
        float blendWidth = max(u_cascadeBlend, (nextSplit - previousSplit) * 0.06);
        float fadeStart = nextSplit - blendWidth;
        
        if (viewDepth > fadeStart) {
            float t = clamp((viewDepth - fadeStart) / u_cascadeBlend, 0.0, 1.0);
            int nextCascade = cascade + 1;
            
            float nextBias = max(u_minDepthBias, u_maxDepthBias * normalLightAngle);
            nextBias *= 1.0 + 0.15 * float(nextCascade);
            vec3 nextOffsetPos = fragPos + normal * (u_normalBias * normalLightAngle);
            vec4 nextLightSpace = u_shadowMatrices[nextCascade] * vec4(nextOffsetPos, 1.0);
            vec3 nextProjCoords = (nextLightSpace.xyz / nextLightSpace.w) * 0.5 + 0.5;
            float nextShadow;

            if (nextProjCoords.x < 0.0 || nextProjCoords.x > 1.0 ||
                nextProjCoords.y < 0.0 || nextProjCoords.y > 1.0 ||
                nextProjCoords.z < 0.0 || nextProjCoords.z > 1.0) {
                nextShadow = shadow;
            } else if (u_penumbraScale > 0.0) {
                nextShadow = PCSSDirectional(nextProjCoords.xy, nextCascade, normal, lightDir, nextProjCoords.z, nextBias);
            } else {
                nextShadow = PCFDirectional(nextProjCoords.xy, nextCascade, nextProjCoords.z, nextBias);
            }
            shadow = mix(shadow, nextShadow, t);
        }
    }

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
    float bias = max(u_minDepthBias, u_maxDepthBias * (1.0 - max(dot(normal, lightDir), 0.0)));
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

float SamplePointShadow(int shadowIndex, vec3 direction)
{
    if (shadowIndex == 0) return texture(u_pointShadowMaps[0], direction).r;
    if (shadowIndex == 1) return texture(u_pointShadowMaps[1], direction).r;
    if (shadowIndex == 2) return texture(u_pointShadowMaps[2], direction).r;
    if (shadowIndex == 3) return texture(u_pointShadowMaps[3], direction).r;
    return 1.0;
}

float PointShadowTexelSize(int shadowIndex)
{
    if (shadowIndex == 0) return 1.0 / float(textureSize(u_pointShadowMaps[0], 0).x);
    if (shadowIndex == 1) return 1.0 / float(textureSize(u_pointShadowMaps[1], 0).x);
    if (shadowIndex == 2) return 1.0 / float(textureSize(u_pointShadowMaps[2], 0).x);
    if (shadowIndex == 3) return 1.0 / float(textureSize(u_pointShadowMaps[3], 0).x);
    return 0.0;
}

float PointShadowCalculation(int lightIndex, vec3 fragPos, vec3 normal, vec3 lightDir)
{
    int shadowIndex = u_pointShadowIndex[lightIndex];
    if (shadowIndex < 0 || shadowIndex >= u_pointShadowCount) return 0.0;

    vec3 lightToFrag = fragPos - u_pointShadowPositions[shadowIndex];
    float distanceToLight = length(lightToFrag);
    float farPlane = max(u_pointShadowFarPlanes[shadowIndex], 0.001);
    if (distanceToLight >= farPlane) return 0.0;
    float currentDepth = distanceToLight / farPlane;
    float bias = max(0.001, 0.003 * (1.0 - max(dot(normal, lightDir), 0.0)));
    vec3 axis = normalize(lightToFrag);
    vec3 tangent = normalize(cross(axis, abs(axis.y) < 0.99 ? vec3(0.0, 1.0, 0.0) : vec3(1.0, 0.0, 0.0)));
    vec3 bitangent = cross(axis, tangent);
    float sampleRadius = 2.0 * PointShadowTexelSize(shadowIndex);
    float shadow = 0.0;

    shadow += currentDepth - bias > SamplePointShadow(shadowIndex, axis) ? 1.0 : 0.0;
    shadow += currentDepth - bias > SamplePointShadow(shadowIndex, normalize(axis + tangent * sampleRadius)) ? 1.0 : 0.0;
    shadow += currentDepth - bias > SamplePointShadow(shadowIndex, normalize(axis - tangent * sampleRadius)) ? 1.0 : 0.0;
    shadow += currentDepth - bias > SamplePointShadow(shadowIndex, normalize(axis + bitangent * sampleRadius)) ? 1.0 : 0.0;
    shadow += currentDepth - bias > SamplePointShadow(shadowIndex, normalize(axis - bitangent * sampleRadius)) ? 1.0 : 0.0;
    return shadow * 0.2;
}

#endif // SHADOWS_GLSL