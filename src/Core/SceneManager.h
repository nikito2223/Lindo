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

        // Инициализация и очистка
        void Init();
        void Update(Input::Input* input);
        void Render(Graphics::Shader& shader);
        void Cleanup();

        // Управление сценами
        template<typename T>
        void RegisterScene(const std::string& name);

        void LoadScene(const std::string& name);
        void UnloadCurrentScene();
        void ReloadCurrentScene();

        Lindo::World::Scene* GetCurrentScene() const { return currentScene.get(); }
        const std::string& GetCurrentSceneName() const { return currentSceneName; }

        // Поиск компонентов и объектов
        template<typename T>
        T* FindComponentInScene() const;

        template<typename T>
        std::vector<T*> FindComponentsInScene() const;

        Lindo::World::GameObject* FindGameObject(const std::string& name) const;
        glm::vec3 GetCharacterPosition() const;

        // Добавление контента напрямую (для быстрого прототипирования)
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

        // Для отложенного добавления контента
        std::vector<std::function<void(Lindo::World::Scene*)>> pendingContent;
        bool isInitialized = false;
    };

    // Реализация шаблонов
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