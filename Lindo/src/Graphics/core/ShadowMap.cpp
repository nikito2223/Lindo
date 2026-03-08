#include "ShadowMap.h"
#include <iostream>

ShadowMap::ShadowMap()
    : FBO(0),
    shadowMap(0),
    depthCubemap(0),
    width(0),
    height(0),
    type(ShadowType::Directional2D)
{
}

ShadowMap::~ShadowMap()
{
    if (FBO) glDeleteFramebuffers(1, &FBO);
    if (shadowMap) glDeleteTextures(1, &shadowMap);
    if (depthCubemap) glDeleteTextures(1, &depthCubemap);
}

bool ShadowMap::Init(unsigned int w,
    unsigned int h,
    ShadowType shadowType)
{
    width = w;
    height = h;
    type = shadowType;

    glGenFramebuffers(1, &FBO);

    if (type == ShadowType::Directional2D)
    {
        glGenTextures(1, &shadowMap);
        glBindTexture(GL_TEXTURE_2D, shadowMap);

        glTexImage2D(GL_TEXTURE_2D,
            0,
            GL_DEPTH_COMPONENT,
            width,
            height,
            0,
            GL_DEPTH_COMPONENT,
            GL_FLOAT,
            nullptr);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

        float borderColor[] = { 1,1,1,1 };
        glTexParameterfv(GL_TEXTURE_2D,
            GL_TEXTURE_BORDER_COLOR,
            borderColor);

        glBindFramebuffer(GL_FRAMEBUFFER, FBO);
        glFramebufferTexture2D(GL_FRAMEBUFFER,
            GL_DEPTH_ATTACHMENT,
            GL_TEXTURE_2D,
            shadowMap,
            0);
    }
    else if (type == ShadowType::PointCube)
    {
        glGenTextures(1, &depthCubemap);
        glBindTexture(GL_TEXTURE_CUBE_MAP, depthCubemap);

        for (unsigned int i = 0; i < 6; ++i)
        {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
                0,
                GL_DEPTH_COMPONENT,
                width,
                height,
                0,
                GL_DEPTH_COMPONENT,
                GL_FLOAT,
                nullptr);
        }

        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

        glBindFramebuffer(GL_FRAMEBUFFER, FBO);
        glFramebufferTexture(GL_FRAMEBUFFER,
            GL_DEPTH_ATTACHMENT,
            depthCubemap,
            0);
    }

    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
        std::cout << "ShadowMap FBO not complete!" << std::endl;
        return false;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    return true;
}

void ShadowMap::BeginRender()
{
    glViewport(0, 0, width, height);
    glBindFramebuffer(GL_FRAMEBUFFER, FBO);
    glClear(GL_DEPTH_BUFFER_BIT);
}

void ShadowMap::EndRender(unsigned int screenWidth,
    unsigned int screenHeight)
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, screenWidth, screenHeight);
}

void ShadowMap::BindForReading(GLenum textureUnit)
{
    glActiveTexture(textureUnit);

    if (type == ShadowType::Directional2D)
        glBindTexture(GL_TEXTURE_2D, shadowMap);
    else
        glBindTexture(GL_TEXTURE_CUBE_MAP, depthCubemap);
}

unsigned int ShadowMap::GetTextureID() const
{
    return (type == ShadowType::Directional2D)
        ? shadowMap
        : depthCubemap;
}