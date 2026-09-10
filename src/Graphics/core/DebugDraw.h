#pragma once

#include <Graphics/core/Shader.h>
#include <glm/glm.hpp>
#include <memory>
#include <vector>

namespace Lindo {
    namespace Graphics {
        class DebugDraw {
        public:

            static DebugDraw& GetInstance() {
                static DebugDraw instance;
                return instance;
            }

            DebugDraw(const DebugDraw&) = delete;
            DebugDraw& operator=(const DebugDraw&) = delete;

            DebugDraw();
            ~DebugDraw();

            void init();
            void begin(const glm::mat4& view, const glm::mat4& projection);

            // Старые методы для кастомных/динамических лучей и капсул
            void DrawLine(const glm::vec3& start, const glm::vec3& end, const glm::vec3& color);
            void DrawCircle(const glm::vec3& center, const glm::vec3& axisA, const glm::vec3& axisB, float radius, const glm::vec3& color, int segments = 24);
            void DrawSphere(const glm::vec3& center, float radius, const glm::vec3& color, int segments = 16);
            void DrawArrow(const glm::vec3& start, const glm::vec3& end, const glm::vec3& color, float headSize = 0.4f);

            // НОВЫЕ: Быстрые GPU-методы
            void DrawWireBox(const glm::mat4& transform, const glm::vec3& color);
            void DrawWireSphereFast(const glm::mat4& transform, const glm::vec3& color);

            void render();
            void clear();

        private:
            void initPrimitives(); // Инициализация единичных мешей

            struct Vertex {
                glm::vec3 position;
                glm::vec3 color;
            };

            std::unique_ptr<Shader> m_shader;

            // Для динамических линий
            unsigned int m_vao = 0;
            unsigned int m_vbo = 0;
            std::vector<Vertex> m_vertices;

            // Для статичных примитивов
            unsigned int m_boxVAO = 0, m_boxVBO = 0;
            unsigned int m_sphereVAO = 0, m_sphereVBO = 0;
            int m_sphereIndexCount = 0;

            glm::mat4 m_view = glm::mat4(1.0f);
            glm::mat4 m_projection = glm::mat4(1.0f);
        };
    }
}