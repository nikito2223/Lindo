#pragma once
#include <string>
#include <vector>
#include <memory>
#include <glm/glm.hpp>
#include "../Graphics/ui/UIRenderer.h"
#include "../Graphics/ui/UIFont.h"
#include "../Graphics/ui/UIWidget.h"

namespace Lindo {
    namespace Debug {

        class DebugOverlay {
        public:
            struct DebugInfo {
                std::string label;
                std::string value;
                Lindo::Graphics::UI::Color color;
            };

            DebugOverlay();
            ~DebugOverlay() = default;

            void init(Lindo::Graphics::UI::UIFont* font);
            
            // Основные обновления
            void update(float deltaTime, int fps, const glm::vec3& playerPos, bool debugMode);
            void updatePosition(const glm::vec3& playerPos);
            void updateStats(float deltaTime, int fps, bool debugMode);
            void setTriangleCount(int count);
            void onResize(int width, int height);

            // Рендеринг через UIRenderer
            void render(Lindo::Graphics::UI::UIRenderer& renderer);

            // Управление видимостью
            void setVisible(bool visible) { m_visible = visible; }
            bool isVisible() const { return m_visible; }
            bool toggle() { m_visible = !m_visible; return m_visible; }

        private:
            void addInfo(const std::string& label, const std::string& value, 
                         const Lindo::Graphics::UI::Color& color = Lindo::Graphics::UI::Color(1, 1, 1, 1));
            void clearInfos();
            void rebuildInfo();

        private:
            Lindo::Graphics::UI::UIFont* m_font = nullptr;
            bool m_visible = false;
            
            float m_updateTimer = 0.0f;
            float m_totalTime = 0.0f;
            int m_currentFPS = 0;
            glm::vec3 m_currentPos{ 0.0f };
            bool m_debugMode = false;
            int m_triangleCount = 0;

            std::vector<DebugInfo> m_infos;

            const float LINE_HEIGHT = 22.0f;
            const float START_X = 12.0f;
            const float START_Y = 12.0f;
        };

    }
}