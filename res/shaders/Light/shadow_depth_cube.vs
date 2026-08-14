#version 330 core
layout (location = 0) in vec3 aPos;

out vec4 FragPos;

uniform mat4 model;
uniform mat4 shadowMatrix;

void main()
{
    FragPos = model * vec4(aPos, 1.0f);
    gl_Position = shadowMatrix * FragPos;
}