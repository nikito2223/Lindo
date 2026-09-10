#pragma once
#include <memory>
#include <unordered_map>
#include <string>
#include <vector>
#include <typeindex>

#include <functional> 
#include <glm/glm.hpp>

#include <world/Scene.h>

namespace Lindo {
    namespace World {
        class GameObject;
        class Component;
    }

    namespace Input {
        class Input;
    }

    namespace Graphics {
        class Shader;
    }

    namespace Components {
        namespace Controller {
            class Player;
        }
    }
}

namespace Lindo {

    class SceneManager {
    public:
        static SceneManager& getInstance();

        // ������������� � �������
        void Init();
        void Update(Input::Input* input);
        void Cleanup();

        // ���������� �������
        template<typename T>
        void RegisterScene(const std::string& name);
        void RegisterSceneFactory(const std::string& name,
            std::function<std::unique_ptr<Lindo::World::Scene>()> creator);

        void LoadScene(const std::string& name);
        void RequestLoadScene(const std::string& name);
        void UnloadCurrentScene();
        void ReloadCurrentScene();

        Lindo::World::Scene* GetCurrentScene() const { return currentScene.get(); }
        const std::string& GetCurrentSceneName() const { return currentSceneName; }
        std::vector<std::string> GetRegisteredSceneNames() const;

        // ����� ����������� � ��������
        template<typename T>
        T* FindComponentInScene() const;

        template<typename T>
        std::vector<T*> FindComponentsInScene() const;

        Lindo::World::GameObject* FindGameObject(const std::string& name) const;
        glm::vec3 GetCharacterPosition() const;

        // ���������� �������� �������� (��� �������� ����������������)
        void AddGameObject(Lindo::World::GameObject* obj);
        void AddContent(std::function<void(Lindo::World::Scene*)> contentCallback);

    private:
        SceneManager() = default;
        ~SceneManager() = default;
        SceneManager(const SceneManager&) = delete;
        SceneManager& operator=(const SceneManager&) = delete;

        void ProcessPendingContent();

        struct SceneInfo {
            std::function<std::unique_ptr<Lindo::World::Scene>()> creator;
            std::string name;
        };

        std::unordered_map<std::string, SceneInfo> registeredScenes;
        std::unique_ptr<Lindo::World::Scene> currentScene;
        std::string currentSceneName;
        std::string pendingSceneName;

        // ��� ����������� ���������� ��������
        std::vector<std::function<void(Lindo::World::Scene*)>> pendingContent;
        bool isInitialized = false;
    };

    // ���������� ��������
    template<typename T>
    void SceneManager::RegisterScene(const std::string& name) {
        registeredScenes[name] = {
            []() -> std::unique_ptr<Lindo::World::Scene> { return std::make_unique<T>(); },
            name
        };
    }

    template<typename T>
    T* SceneManager::FindComponentInScene() const {
        if (!currentScene) return nullptr;
        return currentScene->FindComponentOfType<T>();
    }

    template<typename T>
    std::vector<T*> SceneManager::FindComponentsInScene() const {
        std::vector<T*> result;
        if (!currentScene) return result;

        for (const auto& obj : currentScene->GetGameObjects()) {
            auto* component = obj->getComponent<T>();
            if (component) {
                result.push_back(component);
            }
        }
        return result;
    }
}