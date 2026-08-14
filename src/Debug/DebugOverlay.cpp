#include "DebugOverlay.h"
#include "../Core/Application.h" // Для доступа к AppInfo
#include <sstream>
#include <iomanip>

namespace Lindo {
    namespace Debug {

        DebugOverlay::DebugOverlay() = default;

        void DebugOverlay::init(Lindo::Graphics::UI::UIFont* font) {
            m_font = font;
        }

        void DebugOverlay::update(float deltaTime, int fps, const glm::vec3& playerPos, bool debugMode) {
            m_currentPos = playerPos;
            updateStats(deltaTime, fps, debugMode);
        }

        void DebugOverlay::updatePosition(const glm::vec3& playerPos) {
            m_currentPos = playerPos;
            if (m_visible) {
                rebuildInfo();
            }
        }

        void DebugOverlay::updateStats(float deltaTime, int fps, bool debugMode) {
            m_updateTimer += deltaTime;
            m_totalTime += deltaTime;
            m_currentFPS = fps;
            m_debugMode = debugMode;

            if (m_visible) {
                rebuildInfo();
            }
        }

        void DebugOverlay::setTriangleCount(int count) {
            m_triangleCount = count;
            if (m_visible) {
                rebuildInfo();
            }
        }

        void DebugOverlay::onResize(int width, int height) {
            // При необходимости пересчета UI под новое разрешение
        }

        void DebugOverlay::rebuildInfo() {
            m_infos.clear();

            // 1. Движок и Версия
            addInfo("Engine", AppInfo::Name, Lindo::Graphics::UI::Color(0.2f, 0.8f, 1.0f, 1.0f));
            addInfo("Version", AppInfo::GetVersionString(), Lindo::Graphics::UI::Color(0.7f, 0.7f, 0.7f, 1.0f));

            // 2. FPS & Время кадра (Frametime)
            float frameTimeMs = m_currentFPS > 0 ? (1000.0f / m_currentFPS) : 0.0f;
            std::stringstream fpsStr;
            fpsStr << m_currentFPS << " (" << std::fixed << std::setprecision(1) << frameTimeMs << " ms)";
            
            Lindo::Graphics::UI::Color fpsColor = (m_currentFPS >= 60) 
                ? Lindo::Graphics::UI::Color(0, 1, 0, 1) 
                : ((m_currentFPS >= 30) ? Lindo::Graphics::UI::Color(1, 1, 0, 1) : Lindo::Graphics::UI::Color(1, 0, 0, 1));
            
            addInfo("FPS", fpsStr.str(), fpsColor);

            // 3. Позиция игрока/камеры (X, Y, Z)
            std::stringstream posStr;
            posStr << std::fixed << std::setprecision(2);
            posStr << "X: " << m_currentPos.x << " | Y: " << m_currentPos.y << " | Z: " << m_currentPos.z;
            addInfo("Position", posStr.str());

            // 4. Треугольники (Triangles)
            std::stringstream triStr;
            if (m_triangleCount >= 1000000) {
                triStr << std::fixed << std::setprecision(2) << (m_triangleCount / 1000000.0f) << " M";
            }
            else if (m_triangleCount >= 1000) {
                triStr << std::fixed << std::setprecision(1) << (m_triangleCount / 1000.0f) << " K";
            }
            else {
                triStr << m_triangleCount;
            }
            addInfo("Triangles", triStr.str());

            // 5. Статус отладки физики / гизмо
            addInfo("Debug Mode", m_debugMode ? "ON" : "OFF",
                m_debugMode ? Lindo::Graphics::UI::Color(1, 0.3f, 0.3f, 1) : Lindo::Graphics::UI::Color(0.5f, 0.5f, 0.5f, 1));

            // 6. Uptime (Время работы приложения)
            int minutes = static_cast<int>(m_totalTime) / 60;
            int seconds = static_cast<int>(m_totalTime) % 60;
            std::stringstream timeStr;
            timeStr << std::setfill('0') << std::setw(2) << minutes << ":"
                    << std::setfill('0') << std::setw(2) << seconds;
            addInfo("Uptime", timeStr.str(), Lindo::Graphics::UI::Color(0.8f, 0.8f, 0.8f, 1.0f));
        }

        void DebugOverlay::addInfo(const std::string& label, const std::string& value, const Lindo::Graphics::UI::Color& color) {
            DebugInfo info;
            info.label = label + ":";
            info.value = value;
            info.color = color;
            m_infos.push_back(info);
        }

        void DebugOverlay::clearInfos() {
            m_infos.clear();
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