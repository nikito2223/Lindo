#ifndef LINDO_LIGHTS_GLSL
#define LINDO_LIGHTS_GLSL

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
    float radius;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    bool enabled;
};

#endif