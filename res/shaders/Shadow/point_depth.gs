#version 330 core

layout (triangles) in;
layout (triangle_strip, max_vertices = 18) out;

uniform mat4 u_shadowMatrices[6];

in vec3 vFragPos[];

out vec3 vFragPosOut;

void main()
{
    for (int face = 0; face < 6; ++face)
    {
        gl_Layer = face;
        for (int i = 0; i < 3; ++i)
        {
            vFragPosOut = vFragPos[i];
            gl_Position = u_shadowMatrices[face] * vec4(vFragPos[i], 1.0);
            EmitVertex();
        }
        EndPrimitive();
    }
}
