#ifndef LINDO_MATERIAL_GLSL
#define LINDO_MATERIAL_GLSL

struct Material {
    sampler2D diffuse;
    sampler2D specular;
    vec3 color;
    float shininess;
    bool useTexture;
    vec2 diffuseTiling;
    vec2 diffuseOffset;
    vec2 specularTiling;
    vec2 specularOffset;
};

#endif