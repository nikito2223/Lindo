#pragma once
#include <string>
#include <vector>
#include <glm/glm.hpp>
#include "../Graphics/ui/UIRenderer.h"
#include "../Graphics/ui/UIFont.h"

namespace Lindo {
    namespace Debug {

        class DebugOverlay {
        public:
            struct DebugInfo {
                std::string label;
                std::string value;
                Lindo::Graphics::UI::Color color;
            };

            DebugOverlay() = default;
            ~DebugOverlay() = default;

            void init(Lindo::Graphics::UI::UIFont* font);
            void update(int fps, const glm::vec3& playerPos, float speed);
            void render(Lindo::Graphics::UI::UIRenderer& renderer);

            void setMemoryUsage(float mb) { m_memoryUsageMB = mb; }
            void setSceneObjectCount(int count) { m_sceneObjectCount = count; }

            void setVisible(bool visible) { m_visible = visible; }
            bool isVisible() const { return m_visible; }
            bool toggle() { m_visible = !m_visible; return m_visible; }

        private:
            void rebuildInfo();
            void addInfo(const std::string& label, const std::string& value,
                const Lindo::Graphics::UI::Color& color = Lindo::Graphics::UI::Color(1, 1, 1, 1));

        private:
            std::vector<DebugInfo> m_infos;
            Lindo::Graphics::UI::UIFont* m_font = nullptr;

            bool m_visible = false;
            int m_currentFPS = 0;
            glm::vec3 m_currentPos{ 0.0f };
            float m_currentSpeed = 0.0f;

            float m_memoryUsageMB = 0.0f;
            int m_sceneObjectCount = 0;

            const float LINE_HEIGHT = 22.0f;
            const float START_X = 12.0f;
            const float START_Y = 12.0f;
        };

    }
}