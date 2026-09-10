#include "SceneManager.h"
#include "world/Scene.h"
#include "Component/GameObject/GameObject.h"
#include "Component/PlayerController/Player.h"
#include "Component/Camera/Camera.h"
#include "Component/Physhcs/Colliders/CapsuleCollider.h"
#include <algorithm>
#include "Scripting/LuaApi.h"
#include "Graphics/ui/UIManager.h"

#include "Debug/DebugLogger.h"

namespace Lindo {

    Lindo::SceneManager& Lindo::SceneManager::getInstance() {
        static Lindo::SceneManager instance;
        return instance;
    }

    void Lindo::SceneManager::Init() {
        isInitialized = true;
        LOG_INFO("[SceneManager] Initializing SceneManager...");

        LOG_INFO("[SceneManager] Initialized successfully.");
    }

    void Lindo::SceneManager::RegisterSceneFactory(
        const std::string& name,
        std::function<std::unique_ptr<Lindo::World::Scene>()> creator) {
        if (name.empty() || !creator) return;
        registeredScenes[name] = { std::move(creator), name };
        LOG_INFO("[SceneManager] Registered scene factory: '" + name + "'");
    }

    std::vector<std::string> Lindo::SceneManager::GetRegisteredSceneNames() const {
        std::vector<std::string> names;
        names.reserve(registeredScenes.size());
        for (const auto& entry : registeredScenes) names.push_back(entry.first);
        std::sort(names.begin(), names.end());
        return names;
    }

    void Lindo::SceneManager::Update(Input::Input* input) {
        if (!pendingSceneName.empty()) {
            std::string sceneName = std::move(pendingSceneName);
            pendingSceneName.clear();
            LoadScene(sceneName);
        }

        if (!currentScene) return;

        ProcessPendingContent();

        if (currentScene->IsActive()) {
            currentScene->ProcessInput(input);
            currentScene->Update();
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
        pendingSceneName.clear();
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

        // --- РЕШЕНИЕ 1: АВТОМАТИЧЕСКАЯ ЗАЧИСТКА UI И СБРОС СОСТОЯНИЯ ---
        if (auto* uiManager = Lindo::Scripting::LuaUI::GetManager()) {
            uiManager->clearDynamicWidgets(); // Зачищаем динамические виджеты (панели, кнопки)
        }
        // -------------------------------------------------------------

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
            currentScene->SetName(name);
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

    void Lindo::SceneManager::RequestLoadScene(const std::string& name) {
        if (name.empty()) return;
        pendingSceneName = name;
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

        // Исправлено: добавление объекта в активную сцену
        currentScene->AddGameObject(obj);
    }

    void Lindo::SceneManager::AddContent(std::function<void(Lindo::World::Scene*)> contentCallback) {
        LOG_DEBUG("[SceneManager] Queued dynamic content task for scene.");
        pendingContent.push_back(contentCallback);
    }

    void Lindo::SceneManager::ProcessPendingContent() {
        if (pendingContent.empty()) return;

        // Копируем очереди callbacks, чтобы избежать ошибки при вызове AddContent во время итерации
        auto callbacks = std::move(pendingContent);
        pendingContent.clear();

        LOG_DEBUG("[SceneManager] Processing " + std::to_string(callbacks.size()) + " pending content callbacks...");
        for (auto& callback : callbacks) {
            if (currentScene) {
                callback(currentScene.get());
            }
        }
    }
}