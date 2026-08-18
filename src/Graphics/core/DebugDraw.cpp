#include "DebugDraw.h"
#include <Core/OGL.h>
#include <glm/gtc/type_ptr.hpp>

namespace Lindo {
    namespace Graphics {

        // Обновленные шейдеры
        static const char* debugLineVertex = R"(
#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aColor;

uniform mat4 model; // НОВОЕ: Перенос математики на GPU
uniform mat4 view;
uniform mat4 projection;

out vec3 fragColor;
void main() {
    fragColor = aColor;
    // GPU сама масштабирует и вращает вершины
    gl_Position = projection * view * model * vec4(aPos, 1.0);
}
)";

        static const char* debugLineFragment = R"(
#version 330 core
in vec3 fragColor;

uniform vec3 uColor;
uniform bool uUseUniformColor;

out vec4 FragColor;
void main() {
    if(uUseUniformColor) 
        FragColor = vec4(uColor, 1.0);
    else 
        FragColor = vec4(fragColor, 1.0);
}
)";

        DebugDraw::DebugDraw() = default;

        DebugDraw::~DebugDraw() {
            if (m_vbo) glDeleteBuffers(1, &m_vbo);
            if (m_vao) glDeleteVertexArrays(1, &m_vao);
        }

        void DebugDraw::initPrimitives() {
            // 1. Единичный куб (от -0.5 до 0.5)
            float boxVertices[] = {
                -0.5f,-0.5f,-0.5f,  0.5f,-0.5f,-0.5f,   0.5f,-0.5f,-0.5f,  0.5f, 0.5f,-0.5f,
                 0.5f, 0.5f,-0.5f, -0.5f, 0.5f,-0.5f,  -0.5f, 0.5f,-0.5f, -0.5f,-0.5f,-0.5f,
                -0.5f,-0.5f, 0.5f,  0.5f,-0.5f, 0.5f,   0.5f,-0.5f, 0.5f,  0.5f, 0.5f, 0.5f,
                 0.5f, 0.5f, 0.5f, -0.5f, 0.5f, 0.5f,  -0.5f, 0.5f, 0.5f, -0.5f,-0.5f, 0.5f,
                -0.5f,-0.5f,-0.5f, -0.5f,-0.5f, 0.5f,   0.5f,-0.5f,-0.5f,  0.5f,-0.5f, 0.5f,
                 0.5f, 0.5f,-0.5f,  0.5f, 0.5f, 0.5f,  -0.5f, 0.5f,-0.5f, -0.5f, 0.5f, 0.5f
            };

            glGenVertexArrays(1, &m_boxVAO);
            glGenBuffers(1, &m_boxVBO);
            glBindVertexArray(m_boxVAO);
            glBindBuffer(GL_ARRAY_BUFFER, m_boxVBO);
            glBufferData(GL_ARRAY_BUFFER, sizeof(boxVertices), boxVertices, GL_STATIC_DRAW);
            glEnableVertexAttribArray(0);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

            // 2. Единичная сфера (радиус 1.0)
            std::vector<glm::vec3> spherePoints;
            const int segments = 24;
            for (int i = 0; i < segments; i++) {
                float t1 = (float)i / segments * 6.28318530718f;
                float t2 = (float)(i + 1) / segments * 6.28318530718f;

                // XY Plane
                spherePoints.push_back({ cos(t1), sin(t1), 0 }); spherePoints.push_back({ cos(t2), sin(t2), 0 });
                // XZ Plane
                spherePoints.push_back({ cos(t1), 0, sin(t1) }); spherePoints.push_back({ cos(t2), 0, sin(t2) });
                // YZ Plane
                spherePoints.push_back({ 0, cos(t1), sin(t1) }); spherePoints.push_back({ 0, cos(t2), sin(t2) });
            }
            m_sphereIndexCount = spherePoints.size();

            glGenVertexArrays(1, &m_sphereVAO);
            glGenBuffers(1, &m_sphereVBO);
            glBindVertexArray(m_sphereVAO);
            glBindBuffer(GL_ARRAY_BUFFER, m_sphereVBO);
            glBufferData(GL_ARRAY_BUFFER, spherePoints.size() * sizeof(glm::vec3), spherePoints.data(), GL_STATIC_DRAW);
            glEnableVertexAttribArray(0);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);

            glBindVertexArray(0);
        }

        void DebugDraw::init() {
            m_shader = std::make_unique<Shader>(Shader::FromString(debugLineVertex, debugLineFragment));

            glGenVertexArrays(1, &m_vao);
            glGenBuffers(1, &m_vbo);

            glBindVertexArray(m_vao);
            glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
            glBufferData(GL_ARRAY_BUFFER, 0, nullptr, GL_DYNAMIC_DRAW);

            glEnableVertexAttribArray(0);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
            glEnableVertexAttribArray(1);
            glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, color));

            glBindBuffer(GL_ARRAY_BUFFER, 0);
            glBindVertexArray(0);

            initPrimitives();
        }

        void DebugDraw::begin(const glm::mat4& view, const glm::mat4& projection) {
            m_view = view;
            m_projection = projection;
            m_vertices.clear();
        }

        void DebugDraw::DrawLine(const glm::vec3& start, const glm::vec3& end, const glm::vec3& color) {
            m_vertices.push_back({ start, color });
            m_vertices.push_back({ end, color });
        }

        void DebugDraw::DrawCircle(const glm::vec3& center, const glm::vec3& axisA, const glm::vec3& axisB, float radius, const glm::vec3& color, int segments) {
            glm::vec3 normA = glm::normalize(axisA) * radius;
            glm::vec3 normB = glm::normalize(axisB) * radius;
            glm::vec3 prevPoint = center + normA;

            for (int i = 1; i <= segments; ++i) {
                float theta = 6.28318530718f * float(i) / float(segments);
                glm::vec3 nextPoint = center + cos(theta) * normA + sin(theta) * normB;
                DrawLine(prevPoint, nextPoint, color);
                prevPoint = nextPoint;
            }
        }

        void DebugDraw::DrawSphere(const glm::vec3& center, float radius, const glm::vec3& color, int segments) {
            // Окружности по трем основным плоскостям
            DrawCircle(center, glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), radius, color, segments);
            DrawCircle(center, glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f), radius, color, segments);
            DrawCircle(center, glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f), radius, color, segments);
        }

        void DebugDraw::DrawArrow(const glm::vec3& start, const glm::vec3& end, const glm::vec3& color, float headSize) {
            DrawLine(start, end, color);

            glm::vec3 dir = end - start;
            float len = glm::length(dir);
            if (len < 1e-5f) return;
            dir /= len;

            glm::vec3 right = glm::abs(dir.y) > 0.99f ? glm::vec3(1.0f, 0.0f, 0.0f) : glm::vec3(0.0f, 1.0f, 0.0f);
            glm::vec3 sideA = glm::normalize(glm::cross(dir, right)) * headSize;
            glm::vec3 sideB = glm::normalize(glm::cross(dir, sideA)) * headSize;

            glm::vec3 headBase = end - dir * headSize * 1.5f;

            DrawLine(end, headBase + sideA, color);
            DrawLine(end, headBase - sideA, color);
            DrawLine(end, headBase + sideB, color);
            DrawLine(end, headBase - sideB, color);
        }

        void DebugDraw::DrawWireBox(const glm::mat4& transform, const glm::vec3& color) {
            m_shader->use();
            m_shader->setMat4("view", m_view);
            m_shader->setMat4("projection", m_projection);
            m_shader->setMat4("model", transform);
            m_shader->setVec3("uColor", color);
            m_shader->setInt("uUseUniformColor", 1);

            glBindVertexArray(m_boxVAO);
            glDrawArrays(GL_LINES, 0, 24);
            glBindVertexArray(0);
        }

        void DebugDraw::DrawWireSphereFast(const glm::mat4& transform, const glm::vec3& color) {
            m_shader->use();
            m_shader->setMat4("view", m_view);
            m_shader->setMat4("projection", m_projection);
            m_shader->setMat4("model", transform);
            m_shader->setVec3("uColor", color);
            m_shader->setInt("uUseUniformColor", 1);

            glBindVertexArray(m_sphereVAO);
            glDrawArrays(GL_LINES, 0, m_sphereIndexCount);
            glBindVertexArray(0);
        }

        void DebugDraw::render() {
            if (m_vertices.empty()) return;

            glBindVertexArray(m_vao);
            glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
            glBufferData(GL_ARRAY_BUFFER, m_vertices.size() * sizeof(Vertex), m_vertices.data(), GL_DYNAMIC_DRAW);

            m_shader->use();
            m_shader->setMat4("view", m_view);
            m_shader->setMat4("projection", m_projection);
            m_shader->setMat4("model", glm::mat4(1.0f)); // Сбрасываем модель
            m_shader->setInt("uUseUniformColor", 0);     // Используем цвета вершин

            GLboolean prevCullFace = glIsEnabled(GL_CULL_FACE);
            GLboolean prevDepthTest = glIsEnabled(GL_DEPTH_TEST);

            glDisable(GL_CULL_FACE);
            glDisable(GL_DEPTH_TEST);
            glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(m_vertices.size()));

            if (prevCullFace == GL_TRUE) glEnable(GL_CULL_FACE);
            else glDisable(GL_CULL_FACE);
            if (prevDepthTest == GL_TRUE) glEnable(GL_DEPTH_TEST);
            else glDisable(GL_DEPTH_TEST);

            glBindBuffer(GL_ARRAY_BUFFER, 0);
            glBindVertexArray(0);
        }

        void DebugDraw::clear() {
            m_vertices.clear();
        }
    }
}