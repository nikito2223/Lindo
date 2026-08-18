#pragma once
#include <memory>
#include <glm/glm.hpp>

namespace Lindo {
    class SceneManager;

    namespace Components {
        namespace Rendering { class Camera; }
    }

    namespace Graphics {
        namespace UI {
            class UIManager;
            class UIFont;
        }
    }

    namespace Debug {
        class DebugOverlay;

        class DebugSystem {
        public:
            DebugSystem();
            ~DebugSystem();

            void init(Lindo::Graphics::UI::UIFont* font);

            // Сбор данных и обновление метрик (вызывается в Application::update)
            void update(SceneManager* sceneManager, Lindo::Components::Rendering::Camera* mainCamera);

            // Отрисовка UI-оверлея (вызывается в Renderer во время UI пасса)
            void renderUI(Lindo::Graphics::UI::UIManager* uiManager);

            DebugOverlay* getOverlay() const { return m_overlay.get(); }

        private:
            glm::vec3 resolvePlayerPosition(SceneManager* sceneManager, Lindo::Components::Rendering::Camera* mainCamera);

        private:
            std::unique_ptr<DebugOverlay> m_overlay;

            int m_fps = 0;
            int m_frameCount = 0;
            float m_fpsTimer = 0.0f;
        };
    }
}