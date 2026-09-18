#version 330 core
out vec4 FragColor;

in vec3 TexCoords;

uniform samplerCube skybox;
uniform float u_time;
uniform vec3 u_cameraPos;

void main()
{
    // Небольшое искажение координат от времени для имитации движения верхних слоев атмосферы
    vec3 coords = TexCoords;
    
    vec3 envColor = texture(skybox, coords).rgb;
    
    // Эффект зенита (глубина при взгляде вверх)
    float zenithFactor = clamp(TexCoords.y, 0.0, 1.0);
    envColor *= mix(1.0, 1.2, zenithFactor);

    // Атмосферное дыхание
    float pulse = sin(u_time * 0.4) * 0.02 + 0.98;
    envColor *= pulse;

    // Тон-маппинг (Reinhard) и гамма-коррекция
    envColor = envColor / (envColor + vec3(1.0));
    envColor = pow(envColor, vec3(1.0 / 2.2));

    FragColor = vec4(envColor, 1.0);
}