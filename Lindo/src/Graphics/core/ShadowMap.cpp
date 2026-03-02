#include "ShadowMap.h"
#include <iostream>

ShadowMap::ShadowMap() : FBO(0), shadowMap(0), width(0), height(0) {
    lightViewMatrix = glm::mat4(1.0f);
    lightProjectionMatrix = glm::mat4(1.0f);
}

ShadowMap::~ShadowMap() {
    if (FBO) {
        glDeleteFramebuffers(1, &FBO);
    }
    if (shadowMap) {
        glDeleteTextures(1, &shadowMap);
    }
}

bool ShadowMap::Init(unsigned int width, unsigned int height) {
    this->width = width;
    this->height = height;

    // Создание текстуры глубины
    glGenTextures(1, &shadowMap);
    glBindTexture(GL_TEXTURE_2D, shadowMap);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, width, height, 0,
        GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);

    // Настройка параметров текстуры
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

    // Установка цвета границы (все фрагменты вне карты не в тени)
    float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

    // Создание FBO
    glGenFramebuffers(1, &FBO);
    glBindFramebuffer(GL_FRAMEBUFFER, FBO);

    // Прикрепление текстуры глубины к FBO
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, shadowMap, 0);

    // Отключаем отрисовку цвета
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);

    // Проверка готовности FBO
    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        std::cout << "ERROR::SHADOWMAP: Framebuffer is not complete: " << status << std::endl;
        return false;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    std::cout << "ShadowMap initialized: " << width << "x" << height << std::endl;
    return true;
}

void ShadowMap::BeginRender() {
    glViewport(0, 0, width, height);
    glBindFramebuffer(GL_FRAMEBUFFER, FBO);
    glClear(GL_DEPTH_BUFFER_BIT);

    // Включаем отсечение задних граней для лучшей производительности
    glEnable(GL_CULL_FACE);
    glCullFace(GL_FRONT); // 🔧 Важно: для избежания shadow acne
}

void ShadowMap::EndRender(unsigned int screenWidth, unsigned int screenHeight) {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, screenWidth, screenHeight);
    glDisable(GL_CULL_FACE);
    glCullFace(GL_BACK);
}

void ShadowMap::BindForReading(GLenum textureUnit) {
    glActiveTexture(textureUnit);
    glBindTexture(GL_TEXTURE_2D, shadowMap);
}

void ShadowMap::SetLightMatrices(const glm::mat4& lightView, const glm::mat4& lightProj) {
    lightViewMatrix = lightView;
    lightProjectionMatrix = lightProj;
}