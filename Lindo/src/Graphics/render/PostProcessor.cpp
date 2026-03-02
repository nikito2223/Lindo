#include "PostProcessor.h"
#include <iostream>

PostProcessor::PostProcessor() = default;

PostProcessor::~PostProcessor() {
    if (m_fbo) glDeleteFramebuffers(1, &m_fbo);
    if (m_texColor) glDeleteTextures(1, &m_texColor);
    if (m_texDepth) glDeleteTextures(1, &m_texDepth);
    if (m_vao) glDeleteVertexArrays(1, &m_vao);
    if (m_vbo) glDeleteBuffers(1, &m_vbo);
    delete m_defaultShader;
}

void PostProcessor::init(int width, int height) {
    // Шейдер пост-обработки (по умолчанию простой пасстру)
    m_defaultShader = new Shader(PathData + "shaders/PostProcess/postprocess.vert", PathData + "shaders/PostProcess/postprocess.frag");
    createFramebuffer(width, height);
    createQuad();
}

void PostProcessor::createFramebuffer(int width, int height) {
    m_width = width;
    m_height = height;

    // Создаём FBO
    glGenFramebuffers(1, &m_fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);

    // Цветовая текстура
    glGenTextures(1, &m_texColor);
    glBindTexture(GL_TEXTURE_2D, m_texColor);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_texColor, 0);

    // Текстура глубины
    glGenTextures(1, &m_texDepth);
    glBindTexture(GL_TEXTURE_2D, m_texDepth);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_texDepth, 0);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "PostProcessor: Framebuffer not complete!" << std::endl;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "PostProcessor: Framebuffer not complete! Status: ";
        switch (status) {
        case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT: std::cerr << "Incomplete attachment"; break;
        case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT: std::cerr << "Missing attachment"; break;
        case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER: std::cerr << "Incomplete draw buffer"; break;
        case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER: std::cerr << "Incomplete read buffer"; break;
        case GL_FRAMEBUFFER_UNSUPPORTED: std::cerr << "Unsupported"; break;
        default: std::cerr << "Unknown error " << status; break;
        }
        std::cerr << std::endl;
    }
    else {
        std::cout << "PostProcessor: Framebuffer complete." << std::endl;
    }
}



void PostProcessor::createQuad() {
    float vertices[] = {
        // positions   // texCoords
        -1.0f,  1.0f,  0.0f, 1.0f,
        -1.0f, -1.0f,  0.0f, 0.0f,
         1.0f, -1.0f,  1.0f, 0.0f,

        -1.0f,  1.0f,  0.0f, 1.0f,
         1.0f, -1.0f,  1.0f, 0.0f,
         1.0f,  1.0f,  1.0f, 1.0f
    };

    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);
    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glBindVertexArray(0);
}

void PostProcessor::begin() {
    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void PostProcessor::end() {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void PostProcessor::render(Shader* shader) {
    if (!m_vao || !m_texColor) {
        std::cerr << "PostProcessor: VAO or texture not initialized!" << std::endl;
        return;
    }

    if (!shader) shader = m_defaultShader;
    if (!shader || !shader->isValid()) {
        std::cerr << "PostProcessor: Invalid shader!" << std::endl;
        return;
    }

    // Привязываем шейдер
    shader->use();

    // Привязываем VAO
    glBindVertexArray(m_vao);

    // Активируем и привязываем текстуру
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_texColor);

    // Устанавливаем uniform (ВАЖНО: имя должно совпадать с шейдером)
    GLint loc = glGetUniformLocation(shader->getID(), "screenTexture");
    if (loc != -1) {
        glUniform1i(loc, 0);
    }
    else {
        std::cerr << "PostProcessor: Uniform 'screenTexture' not found!" << std::endl;
    }

    // Рисуем квад
    glDrawArrays(GL_TRIANGLES, 0, 6);

    // Отвязываем
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);

    // Проверяем ошибки
    GLenum err = glGetError();
    if (err != GL_NO_ERROR) {
        std::cerr << "PostProcessor OpenGL error: " << err << std::endl;
    }
}

void PostProcessor::resize(int width, int height) {
    // Пересоздаём текстуры FBO
    glDeleteTextures(1, &m_texColor);
    glDeleteTextures(1, &m_texDepth);
    glDeleteFramebuffers(1, &m_fbo);
    createFramebuffer(width, height);
}