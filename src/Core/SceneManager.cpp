#include "SceneManager.h"
#include "world/Scene.h"
#include "Component/GameObject/GameObject.h"
#include "Component/PlayerController/Player.h"
#include "Component/Camera/Camera.h"
#include "Physics/Collider/CapsuleCollider.h"
#include <iostream>
#include <algorithm>

#include <world/GameScene.h>

namespace Lindo {

    Lindo::SceneManager& Lindo::SceneManager::getInstance() {
        static Lindo::SceneManager instance;
        return instance;
    }

    void Lindo::SceneManager::Init() {
        isInitialized = true;

        // ������������ ����������� ����� (���� �����)
        // RegisterScene<MainMenuScene>("MainMenu");
       RegisterScene<Lindo::Scenes::GameScene>("Game");

        std::cout << "SceneManager initialized" << std::endl;
    }

    void Lindo::SceneManager::Update(float deltaTime, Input::Input* input) {
        if (!currentScene) return;

        ProcessPendingContent();

        if (currentScene->IsActive()) {
            currentScene->Update(deltaTime);
            currentScene->ProcessInput(input, deltaTime);
        }
    }

    void Lindo::SceneManager::Render(Graphics::Shader& shader, float deltaTime) {
        if (!currentScene) return;

        if (currentScene->IsActive()) {
            currentScene->Render(shader, deltaTime);
        }
    }

    void Lindo::SceneManager::Cleanup() {
        if (currentScene) {
            currentScene->OnDeactivate();
            currentScene->OnDestroy();
            currentScene.reset();
        }
        currentSceneName.clear();
        pendingContent.clear();
        isInitialized = false;

        std::cout << "SceneManager cleaned up" << std::endl;
    }

    void Lindo::SceneManager::LoadScene(const std::string& name) {
        auto it = registeredScenes.find(name);
        if (it == registeredScenes.end()) {
            std::cerr << "Scene '" << name << "' not registered!" << std::endl;
            return;
        }

        // ��������� ������� �����
        if (currentScene) {
            currentScene->OnDeactivate();
            currentScene->OnDestroy();
            currentScene.reset();
        }

        // ������� ����� �����
        currentScene = it->second.creator();
        currentSceneName = name;

        if (currentScene) {
            currentScene->OnCreate();
            currentScene->SetActive(true);
            currentScene->OnActivate();

            std::cout << "Loaded scene: " << name << std::endl;
        }
    }

    void Lindo::SceneManager::UnloadCurrentScene() {
        if (currentScene) {
            currentScene->OnDeactivate();
            currentScene->OnDestroy();
            currentScene.reset();
            currentSceneName.clear();
        }
    }

    void Lindo::SceneManager::ReloadCurrentScene() {
        if (!currentSceneName.empty()) {
            LoadScene(currentSceneName);
        }
    }

    Lindo::World::GameObject* Lindo::SceneManager::FindGameObject(const std::string& name) const {
        if (!currentScene) return nullptr;
        return currentScene->FindGameObject(name);
    }

    glm::vec3 Lindo::SceneManager::GetCharacterPosition() const {
        auto* player = FindComponentInScene<Components::Controller::Player>();
        if (player && player->owner) {
            return player->owner->transform.position;
        }
        return glm::vec3(0.0f);
    }

    void Lindo::SceneManager::AddGameObject(Lindo::World::GameObject* obj) {
        if (!currentScene || !obj) return;

        // ���������������! ������ ������� �������� � ����� � ����������:
        /*currentScene->AddGameObject(obj);*/
    }

    void Lindo::SceneManager::AddContent(std::function<void(Lindo::World::Scene*)> contentCallback) {
        pendingContent.push_back(contentCallback);
    }

    void Lindo::SceneManager::ProcessPendingContent() {
        if (pendingContent.empty()) return;

        for (auto& callback : pendingContent) {
            if (currentScene) {
                callback(currentScene.get());
            }
        }
        pendingContent.clear();
    }
}