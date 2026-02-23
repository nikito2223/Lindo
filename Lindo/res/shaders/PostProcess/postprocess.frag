#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D sceneTexture;
uniform sampler2D depthTexture;
uniform float nearPlane;
uniform float farPlane;
uniform float focusDistance; // расстояние фокуса (можно привязать к камере)
uniform float focusRange;    // диапазон резкости

void main() {
    float depth = texture(depthTexture, TexCoords).r;
    // Переводим depth в линейное расстояние
    float z = depth * 2.0 - 1.0;
    float linearDepth = (2.0 * nearPlane * farPlane) / (farPlane + nearPlane - z * (farPlane - nearPlane));
    
    // Вычисляем степень размытия
    float blurAmount = abs(linearDepth - focusDistance) / focusRange;
    blurAmount = clamp(blurAmount, 0.0, 1.0);
    
    // Простое размытие (например, взвешенное по соседям)
    // Для простоты используем box blur или Gaussian blur с переменным радиусом
    // Здесь можно реализовать более сложный алгоритм, но для примера:
    vec2 texelSize = 1.0 / textureSize(sceneTexture, 0);
    float radius = blurAmount * 5.0; // макс радиус 5 пикселей
    
    vec4 color = texture(sceneTexture, TexCoords);
    // Наложить размытие только на области вне фокуса
    // (в реальном коде здесь цикл сэмплов)
    
    FragColor = color;
}