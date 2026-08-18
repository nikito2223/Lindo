#include "SceneManager.h"
#include "world/Scene.h"
#include "Component/GameObject/GameObject.h"
#include "Component/PlayerController/Player.h"
#include "Component/Camera/Camera.h"
#include "Physics/Collider/CapsuleCollider.h"
#include <algorithm>

#include <world/GameScene.h>
#include "Debug/DebugLogger.h"

namespace Lindo {

    Lindo::SceneManager& Lindo::SceneManager::getInstance() {
        static Lindo::SceneManager instance;
        return instance;
    }

    void Lindo::SceneManager::Init() {
        isInitialized = true;

        LOG_INFO("[SceneManager] Initializing SceneManager...");

        // Регистрация доступных сцен
        // RegisterScene<MainMenuScene>("MainMenu");
        RegisterScene<Lindo::Scenes::GameScene>("Game");

        LOG_INFO("[SceneManager] Initialized successfully.");
    }

    void Lindo::SceneManager::Update(Input::Input* input) {
        if (!currentScene) return;

        ProcessPendingContent();

        if (currentScene->IsActive()) {
            currentScene->Update();
            currentScene->ProcessInput(input);
        }
    }

    void Lindo::SceneManager::Render(Graphics::Shader& shader) {
        if (!currentScene) return;

        if (currentScene->IsActive()) {
            currentScene->Render(shader);
        }
    }

    void Lindo::SceneManager::Cleanup() {
        LOG_INFO("[SceneManager] Cleaning up SceneManager...");
        if (currentScene) {
            LOG_INFO("[SceneManager] Unloading scene on cleanup: '" + currentSceneName + "'");
            currentScene->OnDeactivate();
            currentScene->OnDestroy();
            currentScene.reset();
        }
        currentSceneName.clear();
        pendingContent.clear();
        isInitialized = false;

        LOG_INFO("[SceneManager] Cleaned up successfully.");
    }

    void Lindo::SceneManager::LoadScene(const std::string& name) {
        LOG_INFO("[SceneManager] Requested loading scene: '" + name + "'");

        auto it = registeredScenes.find(name);
        if (it == registeredScenes.end()) {
            LOG_ERROR("[SceneManager] Failed to load scene! Scene '" + name + "' is not registered.");
            return;
        }

        // Выгрузка текущей сцены
        if (currentScene) {
            LOG_INFO("[SceneManager] Deactivating and destroying previous scene: '" + currentSceneName + "'");
            currentScene->OnDeactivate();
            currentScene->OnDestroy();
            currentScene.reset();
        }

        // Создание новой сцены
        LOG_DEBUG("[SceneManager] Instantiating scene object for '" + name + "'...");
        currentScene = it->second.creator();
        currentSceneName = name;

        if (currentScene) {
            LOG_INFO("[SceneManager] Calling OnCreate() for scene: '" + name + "'");
            currentScene->OnCreate();

            LOG_INFO("[SceneManager] Activating scene: '" + name + "'");
            currentScene->SetActive(true);
            currentScene->OnActivate();

            LOG_INFO("[SceneManager] Successfully loaded scene: '" + name + "'");
        }
        else {
            LOG_ERROR("[SceneManager] Critical error: Failed to instantiate scene '" + name + "'!");
        }
    }

    void Lindo::SceneManager::UnloadCurrentScene() {
        if (currentScene) {
            LOG_INFO("[SceneManager] Unloading current scene: '" + currentSceneName + "'");
            currentScene->OnDeactivate();
            currentScene->OnDestroy();
            currentScene.reset();
            currentSceneName.clear();
            LOG_INFO("[SceneManager] Scene unloaded.");
        }
        else {
            LOG_WARN("[SceneManager] Attempted to unload current scene, but no scene is currently active.");
        }
    }

    void Lindo::SceneManager::ReloadCurrentScene() {
        if (!currentSceneName.empty()) {
            LOG_INFO("[SceneManager] Reloading current scene: '" + currentSceneName + "'");
            LoadScene(currentSceneName);
        }
        else {
            LOG_WARN("[SceneManager] Attempted to reload scene, but currentSceneName is empty!");
        }
    }

    Lindo::World::GameObject* Lindo::SceneManager::FindGameObject(const std::string& name) const {
        if (!currentScene) {
            LOG_WARN("[SceneManager] Cannot search GameObject '" + name + "' - no active scene.");
            return nullptr;
        }
        return currentScene->FindGameObject(name);
    }

    glm::vec3 Lindo::SceneManager::GetCharacterPosition() const {
        auto* player = FindComponentInScene<Components::Controller::Player>();
        if (player && player->gameObject) {
            return player->gameObject->transform.position;
        }
        return glm::vec3(0.0f);
    }

    void Lindo::SceneManager::AddGameObject(Lindo::World::GameObject* obj) {
        if (!currentScene || !obj) return;
        LOG_DEBUG("[SceneManager] Adding GameObject to active scene...");
        /*currentScene->AddGameObject(obj);*/
    }

    void Lindo::SceneManager::AddContent(std::function<void(Lindo::World::Scene*)> contentCallback) {
        LOG_DEBUG("[SceneManager] Queued dynamic content task for scene.");
        pendingContent.push_back(contentCallback);
    }

    void Lindo::SceneManager::ProcessPendingContent() {
        if (pendingContent.empty()) return;

        LOG_DEBUG("[SceneManager] Processing " + std::to_string(pendingContent.size()) + " pending content callbacks...");
        for (auto& callback : pendingContent) {
            if (currentScene) {
                callback(currentScene.get());
            }
        }
        pendingContent.clear();
    }
}