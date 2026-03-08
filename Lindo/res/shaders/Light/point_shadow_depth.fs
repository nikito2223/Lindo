#version 330 core
in vec4 FragPos;

uniform vec3 lightPos;
uniform float far_plane;

void main()
{
    float lightDistance = length(FragPos.xyz - lightPos);
    // Ќормализуем к [0,1] и записываем как глубину
    gl_FragDepth = lightDistance / far_plane;
}