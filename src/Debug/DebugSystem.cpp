#include "DebugSystem.h"
#include "DebugOverlay.h"
#include "core/SceneManager.h"
#include "graphics/ui/UIManager.h"

#include <Component/GameObject/GameObject.h>
#include <Component/PlayerController/Player.h>
#include "Component/PlayerController/CharacterController.h"
#include <Component/Camera/Camera.h>
#include <Core/Time/Time.h>
#include <Core/FrameManager.h>

#include <windows.h>
#include <psapi.h>
#include <memory>

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

            int currentFPS = static_cast<int>(Lindo::FrameManager::GetFPS());
            glm::vec3 playerPos = resolvePlayerPosition(sceneManager, mainCamera);
            auto* player = sceneManager->FindComponentInScene<Lindo::Components::Controller::Player>();
            float speed = 0.0f;

            if (player && player->GetCharacterController()) {
                speed = player->GetCharacterController()->GetCurrentSpeed();
            }

            m_overlay->update(currentFPS, playerPos, speed);

            // ----- СБОР МЕТРИК -----

            // Память (WorkingSet)
            PROCESS_MEMORY_COUNTERS_EX pmc;
            if (GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc))) {
                float memMB = pmc.WorkingSetSize / (1024.0f * 1024.0f);
                m_overlay->setMemoryUsage(memMB);
            }

            // Количество объектов на сцене
            int objectCount = 0;
            if (sceneManager) {
                auto* scene = sceneManager->GetCurrentScene();
                if (scene) {
                    objectCount = scene->countAllGameObjectsRecursive();
                }
            }

            m_overlay->setSceneObjectCount(objectCount);
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