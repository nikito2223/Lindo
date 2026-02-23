#version 330 core
out vec4 FragColor;

uniform vec3 color;      // цвет, заданный для каждого коллайдера (например, зелёный для платформы, красный для блоков)

void main() {
    FragColor = vec4(color, 1.0); // полностью непрозрачный цвет
}