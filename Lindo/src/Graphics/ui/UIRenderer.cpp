#include "UIRenderer.h"
#include <glad/glad.h>
#include <iostream>
#include <string>

static const char* vertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aTexCoord;
layout (location = 2) in vec4 aColor;

out vec2 TexCoord;
out vec4 Color;

uniform mat4 projection;

void main() {
    gl_Position = projection * vec4(aPos, 0.0, 1.0);
    TexCoord = aTexCoord;
    Color = aColor;
}
)";

static const char* fragmentShaderSource = R"(
#version 330 core
in vec2 TexCoord;
in vec4 Color;
out vec4 FragColor;

uniform sampler2D uTexture;
uniform bool uUseTexture;

void main() {
    if (uUseTexture) {
        // Для текстуры формата GL_RED используем красный канал как альфу
        float alpha = texture(uTexture, TexCoord).r;
        FragColor = vec4(Color.rgb, alpha * Color.a);
    } else {
        FragColor = Color;
    }
}
)";

UIRenderer::UIRenderer() : m_projection(1.0f) {
    m_vertices.reserve(MAX_VERTICES);
}

UIRenderer::~UIRenderer() {
    glDeleteVertexArrays(1, &m_vao);
    glDeleteBuffers(1, &m_vbo);
    glDeleteProgram(m_shaderProgram);
}

bool UIRenderer::init() {
    // Компиляция шейдеров (упрощённо, можно вынести в функцию)
    unsigned int vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, 1, &vertexShaderSource, nullptr);
    glCompileShader(vs);
    unsigned int fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs, 1, &fragmentShaderSource, nullptr);
    glCompileShader(fs);

    m_shaderProgram = glCreateProgram();
    glAttachShader(m_shaderProgram, vs);
    glAttachShader(m_shaderProgram, fs);
    glLinkProgram(m_shaderProgram);

    glDeleteShader(vs);
    glDeleteShader(fs);

    // Создание VAO/VBO
    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);

    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, MAX_VERTICES * sizeof(Vertex), nullptr, GL_DYNAMIC_DRAW);

    // Позиция
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, pos));
    // Текстурные координаты
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, texCoord));
    // Цвет
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, color));

    glBindVertexArray(0);
    return true;
}

void UIRenderer::beginFrame(int screenWidth, int screenHeight) {
    // Ортографическая проекция: лево=0, право=screenWidth, низ=screenHeight, верх=0 (типично для UI)
    m_projection = glm::ortho(0.0f, (float)screenWidth, (float)screenHeight, 0.0f, -1.0f, 1.0f);
    glUseProgram(m_shaderProgram);
    glUniformMatrix4fv(glGetUniformLocation(m_shaderProgram, "projection"), 1, GL_FALSE, &m_projection[0][0]);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
}

void UIRenderer::endFrame() {
    flush();
    glEnable(GL_DEPTH_TEST); // восстанавливаем состояние (опционально)
}

void UIRenderer::drawRect(const Rect& rect, const Color& color) {
    // Вершины для двух треугольников (6 вершин)
    Vertex v0{ {rect.x, rect.y}, {0,0}, {color.r, color.g, color.b, color.a} };
    Vertex v1{ {rect.x + rect.w, rect.y}, {0,0}, {color.r, color.g, color.b, color.a} };
    Vertex v2{ {rect.x + rect.w, rect.y + rect.h}, {0,0}, {color.r, color.g, color.b, color.a} };
    Vertex v3{ {rect.x, rect.y + rect.h}, {0,0}, {color.r, color.g, color.b, color.a} };

    // Первый треугольник
    addVertex(v0);
    addVertex(v1);
    addVertex(v2);
    // Второй треугольник
    addVertex(v0);
    addVertex(v2);
    addVertex(v3);
}

void UIRenderer::drawTexturedRect(const Rect& rect, const Rect& texCoords, unsigned int textureId, const Color& tint)
{
    if (m_currentTexture != textureId || !m_useTexture)
    {
        flush();
        m_currentTexture = textureId;
        m_useTexture = true;
    }

    Vertex v0{ {rect.x, rect.y}, {texCoords.x, texCoords.y}, {tint.r, tint.g, tint.b, tint.a} };
    Vertex v1{ {rect.x + rect.w, rect.y}, {texCoords.x + texCoords.w, texCoords.y}, {tint.r, tint.g, tint.b, tint.a} };
    Vertex v2{ {rect.x + rect.w, rect.y + rect.h}, {texCoords.x + texCoords.w, texCoords.y + texCoords.h}, {tint.r, tint.g, tint.b, tint.a} };
    Vertex v3{ {rect.x, rect.y + rect.h}, {texCoords.x, texCoords.y + texCoords.h}, {tint.r, tint.g, tint.b, tint.a} };

    addVertex(v0);
    addVertex(v1);
    addVertex(v2);
    addVertex(v0);
    addVertex(v2);
    addVertex(v3);


}

void UIRenderer::addVertex(const Vertex& v) {
    m_vertices.push_back(v);
    if (m_vertices.size() >= MAX_VERTICES) {
        flush();
    }
}

void UIRenderer::flush()
{
    if (m_vertices.empty()) return;

    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, m_vertices.size() * sizeof(Vertex), m_vertices.data());

    glUniform1i(glGetUniformLocation(m_shaderProgram, "uUseTexture"), m_useTexture ? 1 : 0);

    if (m_useTexture && m_currentTexture)
    {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, m_currentTexture);
        glUniform1i(glGetUniformLocation(m_shaderProgram, "uTexture"), 0);
    }

    glDrawArrays(GL_TRIANGLES, 0, (GLsizei)m_vertices.size());

    m_vertices.clear();
}

void UIRenderer::drawRaw(const std::vector<Vertex>& vertices, unsigned int textureId)
{
    if (vertices.empty()) return;

    flush(); // закончить предыдущий батч

    glUseProgram(m_shaderProgram);

    glUniform1i(glGetUniformLocation(m_shaderProgram, "uUseTexture"), 1);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textureId);
    glUniform1i(glGetUniformLocation(m_shaderProgram, "uTexture"), 0);

    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);

    glBufferSubData(GL_ARRAY_BUFFER, 0,
        vertices.size() * sizeof(Vertex),
        vertices.data());

    glDrawArrays(GL_TRIANGLES, 0, (GLsizei)vertices.size());

    glUniform1i(glGetUniformLocation(m_shaderProgram, "uUseTexture"), 0);
}