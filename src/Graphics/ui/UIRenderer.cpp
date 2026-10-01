#include "UIRenderer.h"
#include "Core/AssetManager.h"
#include "Graphics/core/Shader.h"
#include <glad/glad.h>
#include <iostream>
#include <string>
#include <debug/DebugLogger.h>
namespace Lindo {
    namespace Graphics {
        namespace UI {
            UIRenderer::UIRenderer() : m_projection(1.0f) {
                m_vertices.reserve(MAX_VERTICES);
            }

            UIRenderer::~UIRenderer() {
                if (m_whiteTexture) glDeleteTextures(1, &m_whiteTexture);
                glDeleteVertexArrays(1, &m_vao);
                glDeleteBuffers(1, &m_vbo);
            }

            bool UIRenderer::init() {
                LOG_INFO("[UIRenderer] Creating OpenGL UI shader program...");
                auto& assets = AssetManager::get();
                const std::string vertexPath = assets.getShaderPath("UI.gslv");
                const std::string fragmentPath = assets.getShaderPath("UI.gslf");
                m_shader = std::make_unique<Lindo::Graphics::Shader>(vertexPath, fragmentPath);
                if (!m_shader->isValid()) {
                    LOG_ERROR("[UIRenderer] Failed to load UI shader program.");
                    return false;
                }

                // �������� VAO/VBO
                glGenVertexArrays(1, &m_vao);
                glGenBuffers(1, &m_vbo);

                glBindVertexArray(m_vao);
                glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
                glBufferData(GL_ARRAY_BUFFER, MAX_VERTICES * sizeof(Vertex), nullptr, GL_DYNAMIC_DRAW);

                // �������
                glEnableVertexAttribArray(0);
                glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, pos));
                // ���������� ����������
                glEnableVertexAttribArray(1);
                glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, texCoord));
                // ����
                glEnableVertexAttribArray(2);
                glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, color));

                // ������ 1x1 ����� �������� ��� ��������� ��������������� ��� ��������
                unsigned char whitePixel[4] = { 255, 255, 255, 255 };
                glGenTextures(1, &m_whiteTexture);
                glBindTexture(GL_TEXTURE_2D, m_whiteTexture);
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, whitePixel);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

                glBindVertexArray(0);
                LOG_INFO(std::string("[UIRenderer] OpenGL UI resources ready. VAO=") +
                    std::to_string(m_vao) + ", VBO=" + std::to_string(m_vbo) +
                    ", WhiteTexture=" + std::to_string(m_whiteTexture));
                return true;
            }

            unsigned int UIRenderer::getShaderProgram() const {
                return m_shader ? m_shader->getID() : 0;
            }

            void UIRenderer::drawRoundedRect(const Rect& rect, const Color& color, float radius, int segments) {
                if (radius <= 0.0f) {
                    drawRect(rect, color);
                    return;
                }
            
                // Ограничиваем радиус половиной минимальной стороны
                radius = std::min(radius, std::min(rect.w, rect.h) * 0.5f);
            
                if (m_currentTexture != m_whiteTexture || !m_useTexture) {
                    flush();
                    m_currentTexture = m_whiteTexture;
                    m_useTexture = true;
                }
            
                std::vector<glm::vec2> points;
                // Центры 4-х дуг скругления
                glm::vec2 centers[4] = {
                    { rect.x + rect.w - radius, rect.y + radius },          // Верхний правый
                    { rect.x + rect.w - radius, rect.y + rect.h - radius },  // Нижний правый
                    { rect.x + radius,          rect.y + rect.h - radius },  // Нижний левый
                    { rect.x + radius,          rect.y + radius }           // Верхний левый
                };
            
                float angles[4] = { 0.0f, glm::half_pi<float>(), glm::pi<float>(), glm::three_over_two_pi<float>() };
            
                for (int i = 0; i < 4; ++i) {
                    for (int j = 0; j <= segments; ++j) {
                        float a = angles[i] + (glm::half_pi<float>() * j / segments);
                        points.push_back(centers[i] + glm::vec2(std::cos(a), std::sin(a)) * radius);
                    }
                }
            
                // Триангуляция скругленного прямоугольника (Triangle Fan от центра)
                glm::vec2 center(rect.x + rect.w * 0.5f, rect.y + rect.h * 0.5f);
                Vertex centerVert{ center, {0,0}, {color.r, color.g, color.b, color.a} };
            
                for (size_t i = 0; i < points.size(); ++i) {
                    size_t next = (i + 1) % points.size();
                    addVertex(centerVert);
                    addVertex(Vertex{ points[i], {0,0}, {color.r, color.g, color.b, color.a} });
                    addVertex(Vertex{ points[next], {0,0}, {color.r, color.g, color.b, color.a} });
                }
            }

            void UIRenderer::beginFrame(int screenWidth, int screenHeight) {
                // ��������������� ��������: ����=0, �����=screenWidth, ���=screenHeight, ����=0 (������� ��� UI)
                m_projection = glm::ortho(0.0f, (float)screenWidth, (float)screenHeight, 0.0f, -1.0f, 1.0f);
                if (!m_shader || !m_shader->isValid()) return;
                m_shader->use();
                glUniformMatrix4fv(glGetUniformLocation(m_shader->getID(), "projection"), 1, GL_FALSE, &m_projection[0][0]);
                glEnable(GL_BLEND);
                glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                glDisable(GL_DEPTH_TEST);
                glDisable(GL_CULL_FACE);
            }

            void UIRenderer::endFrame() {
                flush();
                glEnable(GL_DEPTH_TEST); // ��������������� ��������� (�����������)
            }

            void UIRenderer::drawRaw(const std::vector<Vertex>& vertices, unsigned int textureId)
            {
                if (vertices.empty()) return;
                if (m_currentTexture != textureId || !m_useTexture) {
                    flush();
                    m_currentTexture = textureId;
                    m_useTexture = true;
                }
                // ��������� ��� ������� �����, ������� ���������� push_back
                if (m_vertices.size() + vertices.size() > MAX_VERTICES) {
                    flush(); // ���� �� �������, ���������� � ���������� (����� �������� ������)
                }
                m_vertices.insert(m_vertices.end(), vertices.begin(), vertices.end());
            }


            void UIRenderer::drawRect(const Rect& rect, const Color& color) {
                // ���� ��������� �������� � ������/�������� �� ����� �������� � ������ flush()
                if (m_currentTexture != m_whiteTexture || !m_useTexture) {
                    flush();
                    m_currentTexture = m_whiteTexture;
                    m_useTexture = true;
                }

                Vertex v0{ {rect.x, rect.y}, {0,0}, {color.r, color.g, color.b, color.a} };
                Vertex v1{ {rect.x + rect.w, rect.y}, {0,0}, {color.r, color.g, color.b, color.a} };
                Vertex v2{ {rect.x + rect.w, rect.y + rect.h}, {0,0}, {color.r, color.g, color.b, color.a} };
                Vertex v3{ {rect.x, rect.y + rect.h}, {0,0}, {color.r, color.g, color.b, color.a} };

                addVertex(v0);
                addVertex(v1);
                addVertex(v2);
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

                glActiveTexture(GL_TEXTURE0);

                if (m_useTexture && m_currentTexture != 0)
                {
                    glBindTexture(GL_TEXTURE_2D, m_currentTexture);
                }
                else
                {
                    // ���� �������� �� ������������, ������ ��������� �������� 0 (����� �������� ��������)
                    // ����, ���� � ���� ���� ��������-����� �������, �������� � ID ����.
                    glBindTexture(GL_TEXTURE_2D, 0);
                }

                glUniform1i(glGetUniformLocation(m_shader->getID(), "uTexture"), 0);

                glDrawArrays(GL_TRIANGLES, 0, (GLsizei)m_vertices.size());

                m_vertices.clear();
            }
        }
    }
}