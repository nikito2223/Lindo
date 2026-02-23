#version 410 core
layout (triangles) in;
layout (triangle_strip, max_vertices = 18) out;

uniform mat4 shadowMatrices[6];

out vec4 FragPos; // Фрагментная позиция относительно мировых координат

void main()
{
    for(int face = 0; face < 6; ++face)
    {
        gl_Layer = face; // Встроенная переменная, определяющая грань кубической карты
        for(int i = 0; i < 3; ++i) // Для каждого треугольника вершин
        {
            FragPos = gl_in[i].gl_Position;
            gl_Position = shadowMatrices[face] * FragPos;
            EmitVertex();
        }
        EndPrimitive();
    }
}