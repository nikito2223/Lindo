#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D screenTexture;  // <-- ИЗМЕНЕНО: было sceneTexture
uniform sampler2D depthTexture;
uniform float nearPlane;
uniform float farPlane;
uniform float focusDistance;
uniform float focusRange;

void main() {
    float depth = texture(depthTexture, TexCoords).r;
    // Переводим depth в линейное расстояние
    float z = depth * 2.0 - 1.0;
    float linearDepth = (2.0 * nearPlane * farPlane) / (farPlane + nearPlane - z * (farPlane - nearPlane));
    
    // Вычисляем степень размытия
    float blurAmount = abs(linearDepth - focusDistance) / focusRange;
    blurAmount = clamp(blurAmount, 0.0, 1.0);
    
    // Пока просто выводим исходный цвет для теста
    FragColor = texture(screenTexture, TexCoords);
}