#ifndef SHADOW_MAP_H
#define SHADOW_MAP_H

#include "core/OGL.h"
#include <glm/glm.hpp>

enum class ShadowType
{
    Directional2D,
    PointCube
};

class ShadowMap {
public:
    ShadowMap();
    ~ShadowMap();

    bool Init(unsigned int width,
        unsigned int height,
        ShadowType type);

    void BeginRender();
    void EndRender(unsigned int screenWidth,
        unsigned int screenHeight);

    void BindForReading(GLenum textureUnit);

    unsigned int GetTextureID() const;

    unsigned int GetWidth() const { return width; }
    unsigned int GetHeight() const { return height; }

    ShadowType GetType() const { return type; }

private:
    unsigned int FBO;
    unsigned int shadowMap;      // 2D depth
    unsigned int depthCubemap;   // cube depth
    unsigned int width, height;

    ShadowType type;
};

#endif