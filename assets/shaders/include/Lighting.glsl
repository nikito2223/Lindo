#ifndef LIGHTING_GLSL
#define LIGHTING_GLSL

struct Material {
    sampler2D diffuse;
    sampler2D specular;
    vec3      color;
    float     shininess;
    bool      useTexture;

    // --- UV transform (Unity-style) ---
    vec2      diffuseTiling;
    vec2      diffuseOffset;
    vec2      specularTiling;
    vec2      specularOffset;
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

#endif // LIGHTING_GLSL