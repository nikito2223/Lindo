#version 330 core

in vec3 vFragPosOut;

uniform vec3 u_lightPos;
uniform float u_farPlane;

void main()
{
    float lightDistance = length(vFragPosOut - u_lightPos);
    lightDistance /= u_farPlane;
    gl_FragDepth = lightDistance;
}
