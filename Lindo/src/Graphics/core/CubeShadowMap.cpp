#include "CubeShadowMap.h"
#include <iostream>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>

CubeShadowMap::CubeShadowMap() : FBO(0), depthCubemap(0), size(0) {
    shadowProj = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 25.0f);
}

CubeShadowMap::~CubeShadowMap() {
    if (FBO) glDeleteFramebuffers(1, &FBO);
    if (depthCubemap) glDeleteTextures(1, &depthCubemap);
}

bool CubeShadowMap::Init(unsigned int size) {
    this->size = size;

    // Создаем кубическую текстуру глубины
    glGenTextures(1, &depthCubemap);
    glBindTexture(GL_TEXTURE_CUBE_MAP, depthCubemap);

    for (unsigned int i = 0; i < 6; ++i) {
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_DEPTH_COMPONENT24,
            size, size, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
    }

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    // Создаем FBO
    glGenFramebuffers(1, &FBO);
    glBindFramebuffer(GL_FRAMEBUFFER, FBO);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, depthCubemap, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cout << "ERROR::CUBESHADOWMAP: Framebuffer is not complete!" << std::endl;
        return false;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    std::cout << "CubeShadowMap initialized: " << size << "x" << size << std::endl;
    return true;
}

void CubeShadowMap::BeginRender(int face) {
    glViewport(0, 0, size, size);
    glBindFramebuffer(GL_FRAMEBUFFER, FBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
        GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, depthCubemap, 0);
    glClear(GL_DEPTH_BUFFER_BIT);
}

void CubeShadowMap::EndRender(unsigned int screenWidth, unsigned int screenHeight) {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, screenWidth, screenHeight);
}

void CubeShadowMap::BindForReading(GLenum textureUnit) {
    glActiveTexture(textureUnit);
    glBindTexture(GL_TEXTURE_CUBE_MAP, depthCubemap);
}

std::vector<glm::mat4> CubeShadowMap::GetShadowTransforms(const glm::vec3& lightPos) {
    std::vector<glm::mat4> shadowTransforms;
    glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);

    shadowTransforms.push_back(shadowProj *
        glm::lookAt(lightPos, lightPos + glm::vec3(1.0f, 0.0f, 0.0f), up));
    shadowTransforms.push_back(shadowProj *
        glm::lookAt(lightPos, lightPos + glm::vec3(-1.0f, 0.0f, 0.0f), up));
    shadowTransforms.push_back(shadowProj *
        glm::lookAt(lightPos, lightPos + glm::vec3(0.0f, 1.0f, 0.0f), up));
    shadowTransforms.push_back(shadowProj *
        glm::lookAt(lightPos, lightPos + glm::vec3(0.0f, -1.0f, 0.0f), up));
    shadowTransforms.push_back(shadowProj *
        glm::lookAt(lightPos, lightPos + glm::vec3(0.0f, 0.0f, 1.0f), up));
    shadowTransforms.push_back(shadowProj *
        glm::lookAt(lightPos, lightPos + glm::vec3(0.0f, 0.0f, -1.0f), up));

    return shadowTransforms;
}