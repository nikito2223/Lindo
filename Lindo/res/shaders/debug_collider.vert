#version 330 core
layout (location = 0) in vec3 aPos;   // позиция вершины в локальном пространстве коллайдера

uniform mat4 model;      // матрица модели (трансформация коллайдера)
uniform mat4 view;       // видовая матрица камеры
uniform mat4 projection; // матрица проекции

void main() {
    gl_Position = projection * view * model * vec4(aPos, 1.0);
}