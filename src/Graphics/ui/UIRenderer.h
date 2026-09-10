#pragma once
#include "UIMath.h"
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace Lindo {
    namespace Graphics {
        namespace UI {
            class UIRenderer {
            public:
                UIRenderer();
                ~UIRenderer();

                struct Vertex {
                    glm::vec2 pos;
                    glm::vec2 texCoord;
                    glm::vec4 color;
                };

                // ������������� �������� � �������
                bool init();

                // ������ ����: ���������� ��������������� ��������
                void beginFrame(int screenWidth, int screenHeight);

                // ���������� ����������� �������������
                void drawRect(const Rect& rect, const Color& color);

                // ���������� ������������� � ��������� (��� ����������� ��� ������)
                void drawTexturedRect(const Rect& rect, const Rect& texCoords, unsigned int textureId, const Color& tint = Color(1, 1, 1, 1));

                // ��������� ����: ��������� ���������� ���������
                void endFrame();

                void drawRaw(const std::vector<Vertex>& vertices, unsigned int textureId);
                unsigned int getShaderProgram() const { return m_shaderProgram; }

                unsigned int m_currentTexture = 0;
                bool m_useTexture = false;
                void drawRoundedRect(const Rect& rect, const Color& color, float cornerRadius, int segmentsPerCorner = 6);

            private:
                static const size_t MAX_VERTICES = 100000;
                std::vector<Vertex> m_vertices;
                unsigned int m_vao, m_vbo;
                unsigned int m_shaderProgram;
                glm::mat4 m_projection;
                unsigned int m_whiteTexture = 0;

                void flush(); // ��������� ����������� ������� � OpenGL
                void addVertex(const Vertex& v);
            };
        }
    }
}