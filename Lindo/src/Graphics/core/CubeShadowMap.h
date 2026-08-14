#ifndef CUBE_SHADOW_MAP_H
#define CUBE_SHADOW_MAP_H

#include "core/OGL.h"
#include <glm/glm.hpp>
#include <vector>

class CubeShadowMap {
public:
    CubeShadowMap();
    ~CubeShadowMap();

    bool Init(unsigned int size = 1024);
    void BeginRender(int face);
    void EndRender(unsigned int screenWidth, unsigned int screenHeight);
    void BindForReading(GLenum textureUnit);

    // Получение матриц проекции для всех 6 сторон
    std::vector<glm::mat4> GetShadowTransforms(const glm::vec3& lightPos);

    unsigned int GetTextureID() const { return depthCubemap; }
    unsigned int GetSize() const { return size; }

private:
    unsigned int FBO;
    unsigned int depthCubemap;
    unsigned int size;

    glm::mat4 shadowProj;
};

#endif