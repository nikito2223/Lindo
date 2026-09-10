#include "DebugOverlay.h"
#include "../Core/Application.h"
#include <sstream>
#include <iomanip>
#include <Core/Time/Time.h>
#include "Graphics/ui/UIWidget.h"

namespace Lindo {
    namespace Debug {

        void DebugOverlay::init(Lindo::Graphics::UI::UIFont* font) {
            m_font = font;
        }

        void DebugOverlay::update(int fps, const glm::vec3& playerPos, float speed) {
            m_currentFPS = fps;
            m_currentPos = playerPos;
            m_currentSpeed = speed;

            if (m_visible) {
                rebuildInfo();
            }
        }

        void DebugOverlay::rebuildInfo() {
            m_infos.clear();

            // Engine & Version
            addInfo("Engine", AppInfo::Name, Lindo::Graphics::UI::Color(0.2f, 0.8f, 1.0f, 1.0f));
            addInfo("Version", AppInfo::GetVersionString(), Lindo::Graphics::UI::Color(0.7f, 0.7f, 0.7f, 1.0f));

            // FPS & Frame Time
            float frameTimeMs = m_currentFPS > 0 ? (1000.0f / m_currentFPS) : 0.0f;
            std::stringstream fpsStr;
            fpsStr << m_currentFPS << " (" << std::fixed << std::setprecision(1) << frameTimeMs << " ms)";
            Lindo::Graphics::UI::Color fpsColor = (m_currentFPS >= 60) ? Lindo::Graphics::UI::Color(0, 1, 0, 1)
                : ((m_currentFPS >= 30) ? Lindo::Graphics::UI::Color(1, 1, 0, 1)
                    : Lindo::Graphics::UI::Color(1, 0, 0, 1));
            addInfo("FPS", fpsStr.str(), fpsColor);

            // Position
            std::stringstream posStr;
            posStr << std::fixed << std::setprecision(2) << "X: " << m_currentPos.x << " | Y: " << m_currentPos.y << " | Z: " << m_currentPos.z;
            addInfo("Position", posStr.str());

            // Speed
            std::stringstream speedStr;
            speedStr << std::fixed << std::setprecision(2) << m_currentSpeed << " m/s";
            addInfo("Speed", speedStr.str(), Lindo::Graphics::UI::Color(0.2f, 0.9f, 0.9f, 1.0f));

            // Memory
            std::stringstream memStr;
            memStr << std::fixed << std::setprecision(1) << m_memoryUsageMB << " MB";
            addInfo("Memory", memStr.str(), Lindo::Graphics::UI::Color(0.6f, 0.9f, 0.6f, 1.0f));

            // Objects Count
            addInfo("Scene Objects", std::to_string(m_sceneObjectCount), Lindo::Graphics::UI::Color(1.0f, 0.8f, 0.2f, 1.0f));
        }

        void DebugOverlay::addInfo(const std::string& label, const std::string& value, const Lindo::Graphics::UI::Color& color) {
            m_infos.push_back({ label + ":", value, color });
        }

        void DebugOverlay::render(Lindo::Graphics::UI::UIRenderer& renderer) {
            if (!m_visible || !m_font) return;

            float y = START_Y;
            for (const auto& info : m_infos) {
                std::string fullText = info.label + " " + info.value;

                Lindo::Graphics::UI::UILabel tempLabel(fullText);
                tempLabel.setPosition(START_X, y);
                tempLabel.setTextSize(16.0f);
                tempLabel.setTextColor(info.color);
                tempLabel.render(renderer, m_font);

                y += LINE_HEIGHT;
            }
        }

    }
}