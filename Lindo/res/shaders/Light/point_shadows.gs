#version 330 core
layout (triangles) in;
layout (triangle_strip, max_vertices=18) out;

uniform mat4 shadowMatrices[6]; // Матрицы для каждой грани (проекция * вид)

out vec4 FragPos; // Мировая позиция фрагмента (для фрагментного шейдера)

void main()
{
    for(int face = 0; face < 6; ++face)
    {
        gl_Layer = face; // Выбираем грань куба
        for(int i = 0; i < 3; ++i) // Для каждой вершины треугольника
        {
            FragPos = gl_in[i].gl_Position; // Мировая позиция
            gl_Position = shadowMatrices[face] * FragPos;
            EmitVertex();
        }
        EndPrimitive();
    }
}