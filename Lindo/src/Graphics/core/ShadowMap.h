#ifndef SHADOW_MAP_H
#define SHADOW_MAP_H

#include "core/OGL.h"
#include <glm/glm.hpp>

class ShadowMap {
public:
    ShadowMap();
    ~ShadowMap();

    // Инициализация карты теней
    bool Init(unsigned int width = 2048, unsigned int height = 2048);

    // Начало записи в карту теней
    void BeginRender();

    // Конец записи
    void EndRender(unsigned int screenWidth, unsigned int screenHeight);

    // Чтение карты теней
    void BindForReading(GLenum textureUnit);

    // Установка матриц света
    void SetLightMatrices(const glm::mat4& lightView, const glm::mat4& lightProj);

    // Получение матриц
    glm::mat4 GetLightView() const { return lightViewMatrix; }
    glm::mat4 GetLightProjection() const { return lightProjectionMatrix; }
    glm::mat4 GetLightSpaceMatrix() const { return lightProjectionMatrix * lightViewMatrix; }

    // Получение текстуры
    unsigned int GetTextureID() const { return shadowMap; }

    // Получение размеров
    unsigned int GetWidth() const { return width; }
    unsigned int GetHeight() const { return height; }

private:
    unsigned int FBO;        // Framebuffer Object
    unsigned int shadowMap;  // Текстура глубины
    unsigned int width, height;

    glm::mat4 lightViewMatrix;
    glm::mat4 lightProjectionMatrix;
};

#endif