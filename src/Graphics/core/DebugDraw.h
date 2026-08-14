#pragma once

#include <Graphics/core/Shader.h>
#include <glm/glm.hpp>
#include <memory>
#include <vector>

namespace Lindo {
    namespace Graphics {
        class DebugDraw {
        public:
            DebugDraw();
            ~DebugDraw();

            void init();
            void begin(const glm::mat4& view, const glm::mat4& projection);
            void DrawLine(const glm::vec3& start, const glm::vec3& end, const glm::vec3& color);
            void DrawCircle(const glm::vec3& center, const glm::vec3& axisA, const glm::vec3& axisB, float radius, const glm::vec3& color, int segments = 24);
            void DrawSphere(const glm::vec3& center, float radius, const glm::vec3& color, int segments = 16);
            void DrawArrow(const glm::vec3& start, const glm::vec3& end, const glm::vec3& color, float headSize = 0.4f);
            void render();
            void clear();

        private:
            struct Vertex {
                glm::vec3 position;
                glm::vec3 color;
            };

            std::unique_ptr<Shader> m_shader;
            unsigned int m_vao = 0;
            unsigned int m_vbo = 0;
            std::vector<Vertex> m_vertices;
            glm::mat4 m_view = glm::mat4(1.0f);
            glm::mat4 m_projection = glm::mat4(1.0f);
        };
    }
}