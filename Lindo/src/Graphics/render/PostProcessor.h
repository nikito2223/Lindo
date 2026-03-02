#pragma once
#include <core/OGL.h>
#include <glm/glm.hpp>
#include <Graphics/core/Shader.h>

class PostProcessor {
public:
    PostProcessor();
    ~PostProcessor();

    void init(int width, int height);
    void begin(); // включаем рендер в FBO
    void end();   // возвращаем рендер в основной буфер
    void render(Shader* shader = nullptr); // отрисовка полноэкранного квада с текстурой

    // Изменение размера
    void resize(int width, int height);

    int getWidth() const { return m_width; }
    int getHeight() const { return m_height; }

private:
    void createFramebuffer(int width, int height);
    void createQuad();

    unsigned int m_fbo = 0;
    unsigned int m_texColor = 0;
    unsigned int m_texDepth = 0;
    unsigned int m_vao = 0;
    unsigned int m_vbo = 0;
    int m_width = 0, m_height = 0;

    Shader* m_defaultShader = nullptr; // шейдер пост-обработки
};