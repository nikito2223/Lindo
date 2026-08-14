#include "DebugDraw.h"
#include <Core/OGL.h>
#include <glm/gtc/type_ptr.hpp>

namespace Lindo {
    namespace Graphics {

        static const char* debugLineVertex = R"(
#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aColor;
uniform mat4 view;
uniform mat4 projection;
out vec3 fragColor;
void main() {
    fragColor = aColor;
    gl_Position = projection * view * vec4(aPos, 1.0);
}
)";

        static const char* debugLineFragment = R"(
#version 330 core
in vec3 fragColor;
out vec4 FragColor;
void main() {
    FragColor = vec4(fragColor, 1.0);
}
)";

        DebugDraw::DebugDraw() = default;

        DebugDraw::~DebugDraw() {
            if (m_vbo) glDeleteBuffers(1, &m_vbo);
            if (m_vao) glDeleteVertexArrays(1, &m_vao);
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

        void DebugDraw::render() {
            if (m_vertices.empty()) return;

            glBindVertexArray(m_vao);
            glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
            glBufferData(GL_ARRAY_BUFFER, m_vertices.size() * sizeof(Vertex), m_vertices.data(), GL_DYNAMIC_DRAW);

            m_shader->use();
            m_shader->setMat4("view", m_view);
            m_shader->setMat4("projection", m_projection);

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