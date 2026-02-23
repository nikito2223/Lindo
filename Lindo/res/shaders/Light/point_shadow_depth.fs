#version 410 core
in vec4 FragPos;

uniform vec3 lightPos;
uniform float far_plane;

void main()
{
    // Получаем расстояние между фрагментом и источником света
    float lightDistance = length(FragPos.xyz - lightPos);
    
    // Делим на far_plane для отображения в диапазоне [0;1]
    lightDistance = lightDistance / far_plane;
    
    // Записываем это как глубину фрагмента
    gl_FragDepth = lightDistance;
}