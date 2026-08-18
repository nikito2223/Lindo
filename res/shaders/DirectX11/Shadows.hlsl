#ifndef SHADOWS_HLSL
#define SHADOWS_HLSL

#define MAX_CASCADES 4

// === CSM ===
Texture2DArray gShadowMap      : register(t2);
SamplerState   gShadowSampler  : register(s1);

cbuffer CSMData : register(b4)
{
    bool   u_shadowsEnabled;
    int    u_cascadeCount;
    float  u_cascadeSplitPlanes[MAX_CASCADES];
    float4x4 u_shadowMatrices[MAX_CASCADES];
    float  u_shadowMapSize;
    float  u_penumbraScale;
    float  u_cascadeBlend;
}

// === Point Shadows ===
TextureCube gPointShadowMaps[4] : register(t3);

cbuffer PointShadowData : register(b5)
{
    float u_pointShadowFarPlanes[4];
    float3 u_pointShadowPositions[4];
    int   u_pointShadowCount;
}

// === Spot Shadows ===
Texture2D   gSpotShadowMap     : register(t7);
SamplerState gSpotShadowSampler: register(s2);

cbuffer SpotShadowData : register(b6)
{
    bool    u_spotShadowsEnabled;
    float4x4 u_spotShadowMatrix;
    float3   u_spotLightPosition;
    float3   u_spotLightDirection;
    float    u_spotShadowFarPlane;
}

// Глобальный массив направлений выборки
static const float3 sampleOffsetDirections[20] = {
    float3( 1,  1,  1), float3( 1, -1,  1), float3(-1, -1,  1), float3(-1,  1,  1),
    float3( 1,  1, -1), float3( 1, -1, -1), float3(-1, -1, -1), float3(-1,  1, -1),
    float3( 1,  1,  0), float3( 1, -1,  0), float3(-1, -1,  0), float3(-1,  1,  0),
    float3( 1,  0,  1), float3(-1,  0,  1), float3( 1,  0, -1), float3(-1,  0, -1),
    float3( 0,  1,  1), float3( 0, -1,  1), float3( 0,  1, -1), float3( 0, -1, -1)
};

int selectCascade(float viewDepth)
{
    int cascade = 0;
    [loop]
    for (int i = 0; i < u_cascadeCount - 1; ++i) {
        if (viewDepth > u_cascadeSplitPlanes[i]) {
            cascade = i + 1;
        }
    }
    return clamp(cascade, 0, u_cascadeCount - 1);
}

float PCFDirectional(float2 projCoords, int cascade, float currentDepth, float bias)
{
    float2 texelSize = 1.0 / float2(gShadowMap.GetDimensions().xy);
    float shadow = 0.0;
    int kernel = 2;

    [loop]
    for (int x = -kernel; x <= kernel; ++x) {
        for (int y = -kernel; y <= kernel; ++y) {
            float2 offset = float2(x, y) * texelSize;
            float pcfDepth = gShadowMap.Sample(gShadowSampler,
                                               float3(projCoords + offset, cascade)).r;
            shadow += (currentDepth - bias) > pcfDepth ? 1.0 : 0.0;
        }
    }
    return shadow / 25.0;
}

float BlockerSearch(float2 projCoords, int cascade, float currentDepth, float bias, float searchRadius)
{
    float2 texelSize = 1.0 / float2(gShadowMap.GetDimensions().xy);
    float blockerDepth = 0.0;
    int count = 0;

    [loop]
    for (int x = -2; x <= 2; ++x) {
        for (int y = -2; y <= 2; ++y) {
            float2 offset = float2(x, y) * texelSize * searchRadius;
            float pcfDepth = gShadowMap.Sample(gShadowSampler,
                                               float3(projCoords + offset, cascade)).r;
            if ((currentDepth - bias) > pcfDepth) {
                blockerDepth += pcfDepth;
                count++;
            }
        }
    }
    if (count == 0) return -1.0;
    return blockerDepth / float(count);
}

float PCSSDirectional(float2 projCoords, int cascade, float3 normal, float3 lightDir,
                      float currentDepth, float bias)
{
    float2 texelSize = 1.0 / float2(gShadowMap.GetDimensions().xy);
    float scale = u_penumbraScale;
    float searchRadius = 2.0 * scale;

    float avgBlockerDepth = BlockerSearch(projCoords, cascade, currentDepth, bias, searchRadius);
    if (avgBlockerDepth < 0.0) return 0.0;

    float penumbra = (currentDepth - avgBlockerDepth) / avgBlockerDepth;
    float filterRadius = clamp(penumbra * scale * 3.0, 0.0, 8.0);

    float shadow = 0.0;
    int kernel = 3;

    [loop]
    for (int x = -kernel; x <= kernel; ++x) {
        for (int y = -kernel; y <= kernel; ++y) {
            float2 offset = float2(x, y) * texelSize * filterRadius;
            float pcfDepth = gShadowMap.Sample(gShadowSampler,
                                               float3(projCoords + offset, cascade)).r;
            shadow += (currentDepth - bias) > pcfDepth ? 1.0 : 0.0;
        }
    }
    return shadow / 49.0;
}

float CascadedShadowCalculation(float3 fragPos, float3 normal, float3 lightDir, float viewDepth)
{
    if (!u_shadowsEnabled) return 0.0;

    int cascade = selectCascade(viewDepth);
    float normalBiasScale = 0.02;
    float3 offsetFragPos = fragPos + normal * normalBiasScale;

    float4 fragPosLightSpace = mul(u_shadowMatrices[cascade], float4(offsetFragPos, 1.0));
    float3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;

    if (projCoords.z > 1.0 || projCoords.z < 0.0 ||
        projCoords.x < 0.0 || projCoords.x > 1.0 ||
        projCoords.y < 0.0 || projCoords.y > 1.0) {
        return 0.0;
    }

    float currentDepth = projCoords.z;
    float bias = max(0.0005, 0.003 * (1.0 - dot(normal, lightDir)));

    float shadow = (u_penumbraScale > 0.0)
        ? PCSSDirectional(projCoords.xy, cascade, normal, lightDir, currentDepth, bias)
        : PCFDirectional(projCoords.xy, cascade, currentDepth, bias);

    float blend = u_cascadeBlend;
    if (cascade > 0 && viewDepth > u_cascadeSplitPlanes[cascade - 1]) {
        float t = clamp((viewDepth - u_cascadeSplitPlanes[cascade - 1]) / blend, 0.0, 1.0);
        shadow *= (1.0 - t);
    }

    return saturate(shadow);
}

float PointShadowCalculation(float3 fragPos, float3 normal, float3 lightPos, int shadowIndex)
{
    if (shadowIndex < 0 || shadowIndex >= u_pointShadowCount) return 0.0;

    float3 fragToLight = fragPos - lightPos;
    float currentDepth = length(fragToLight);
    float farPlane = u_pointShadowFarPlanes[shadowIndex];

    if (currentDepth > farPlane) return 0.0;

    float3 lightDir = normalize(lightPos - fragPos);
    float bias = max(0.05 * (1.0 - dot(normal, lightDir)), 0.005);

    float shadow = 0.0;
    float diskRadius = 0.05;

    [loop]
    for (int i = 0; i < 20; ++i) {
        float3 sampleDir = fragToLight + sampleOffsetDirections[i] * diskRadius;
        float closest = gPointShadowMaps[shadowIndex].Sample(gShadowSampler, sampleDir).r;
        closest *= farPlane;
        if (currentDepth - bias > closest) shadow += 1.0;
    }
    shadow /= 20.0;

    return saturate(shadow);
}

float SpotShadowCalculation(float3 fragPos, float3 normal, float3 lightPos, float3 lightDir)
{
    if (!u_spotShadowsEnabled) return 0.0;

    float4 fragPosLightSpace = mul(u_spotShadowMatrix, float4(fragPos, 1.0));
    float3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;

    if (projCoords.z > 1.0 || projCoords.z < 0.0 ||
        projCoords.x < 0.0 || projCoords.x > 1.0 ||
        projCoords.y < 0.0 || projCoords.y > 1.0) {
        return 0.0;
    }

    float currentDepth = projCoords.z;
    float bias = max(0.005, 0.02 * (1.0 - dot(normal, lightDir)));

    uint w, h;
    gSpotShadowMap.GetDimensions(w, h);
    float2 texelSize = 1.0 / float2(w, h);

    float shadow = 0.0;
    int kernel = 1;

    [loop]
    for (int x = -kernel; x <= kernel; ++x) {
        for (int y = -kernel; y <= kernel; ++y) {
            float2 offset = float2(x, y) * texelSize;
            float pcfDepth = gSpotShadowMap.Sample(gSpotShadowSampler,
                                                   projCoords.xy + offset).r;
            shadow += (currentDepth - bias) > pcfDepth ? 1.0 : 0.0;
        }
    }
    shadow /= float((2 * kernel + 1) * (2 * kernel + 1));

    if (length(fragPos - lightPos) > u_spotShadowFarPlane) shadow = 0.0;
    return saturate(shadow);
}

#endif // SHADOWS_HLSL
