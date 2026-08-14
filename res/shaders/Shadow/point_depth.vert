#version 330 core

layout (location = 0) in vec3 aPos;

uniform mat4 u_model;

out vec3 vFragPos;

void main()
{
    vFragPos = (u_model * vec4(aPos, 1.0)).xyz;
    // gl_Position is set by the geometry shader.
}
