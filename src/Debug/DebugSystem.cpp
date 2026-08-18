#include "DebugSystem.h"
#include "DebugOverlay.h"
#include "core/SceneManager.h"
#include "graphics/ui/UIManager.h"

#include <Component/GameObject/GameObject.h>
#include <Component/PlayerController/Player.h>
#include <Component/Camera/Camera.h>
#include <Core/Time/Time.h>
#include <Core/FrameManager.h> // Подключаем FrameManager

namespace Lindo {
    namespace Debug {

        DebugSystem::DebugSystem()
            : m_overlay(std::make_unique<DebugOverlay>()) {
        }

        DebugSystem::~DebugSystem() = default;

        void DebugSystem::init(Lindo::Graphics::UI::UIFont* font) {
            if (m_overlay) {
                m_overlay->init(font);
            }
        }

        void DebugSystem::update(SceneManager* sceneManager, Lindo::Components::Rendering::Camera* mainCamera) {
            if (!m_overlay || !m_overlay->isVisible()) return;

            // 1. Забираем точный сглаженный FPS из FrameManager без искусственной задержки в 1 секунду
            int currentFPS = static_cast<int>(Lindo::FrameManager::GetFPS());

            // 2. Получение координаты игрока/камеры
            glm::vec3 playerPos = resolvePlayerPosition(sceneManager, mainCamera);

            // 3. Обновление отладочной информации каждый кадр
            m_overlay->update(currentFPS, playerPos, true);
        }

        glm::vec3 DebugSystem::resolvePlayerPosition(SceneManager* sceneManager, Lindo::Components::Rendering::Camera* mainCamera) {
            Lindo::Components::Controller::Player* player = nullptr;

            if (mainCamera && mainCamera->gameObject) {
                player = mainCamera->gameObject->getComponentInParent<Lindo::Components::Controller::Player>();
            }

            if (!player && sceneManager) {
                player = sceneManager->FindComponentInScene<Lindo::Components::Controller::Player>();
            }

            if (player && player->gameObject) {
                return player->gameObject->getWorldPosition();
            }

            if (mainCamera && mainCamera->gameObject) {
                return mainCamera->gameObject->getWorldPosition();
            }

            return glm::vec3(0.0f);
        }

        void DebugSystem::renderUI(Lindo::Graphics::UI::UIManager* uiManager) {
            if (!m_overlay || !m_overlay->isVisible() || !uiManager) return;

            auto* uiRenderer = uiManager->getRenderer();
            if (uiRenderer) {
                m_overlay->render(*uiRenderer);
            }
        }

    }
}