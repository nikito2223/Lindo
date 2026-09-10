#pragma once
#include <glm/glm.hpp>
#include <vector>
#include <memory>
#include <functional>
#include <typeindex>
#include <string>
#include <unordered_map>

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
}

namespace Lindo {
    namespace World {

        class Scene {
        public:
            Scene() = default;
            virtual ~Scene();

            // Жизненный цикл сцены
            virtual void OnCreate() {}      // При создании сцены
            virtual void OnActivate() {}    // Когда сцена становится активной
            virtual void OnDeactivate() {}  // Когда сцена деактивируется
            virtual void OnDestroy() {}     // При уничтожении сцены

            // Основные циклы
            virtual void Update() {}
            virtual void ProcessInput(Input::Input* input) {}

            // Управление GameObject
            Lindo::World::GameObject* CreateGameObject(const std::string& name = "GameObject");
            void AddGameObject(Lindo::World::GameObject* obj); // Добавление уже созданного объекта
            void DestroyGameObject(Lindo::World::GameObject* obj);
            void DestroyGameObject(const std::string& name);

            // Поиск объектов
            Lindo::World::GameObject* FindGameObject(const std::string& name) const;
            std::vector<Lindo::World::GameObject*> FindGameObjectsByTag(const std::string& tag) const;

            template<typename T>
            T* FindComponentOfType() const;

            template<typename T>
            std::vector<T*> FindComponentsOfType() const;

            // Геттеры
            const std::vector<std::unique_ptr<Lindo::World::GameObject>>& GetGameObjects() const { return gameObjects; }
            int countAllGameObjectsRecursive() const;

            bool IsActive() const { return isActive; }
            const std::string& GetName() const { return sceneName; }

            // Сеттеры
            void SetName(const std::string& name) { sceneName = name; }
            void SetActive(bool active) { isActive = active; }

        protected:
            std::vector<std::unique_ptr<Lindo::World::GameObject>> gameObjects;
            std::string sceneName = "Untitled Scene";
            bool isActive = false;

        private:
            std::unordered_map<std::string, Lindo::World::GameObject*> gameObjectMap;
        };

        // Реализация шаблонов поиска
        template<typename T>
        T* Scene::FindComponentOfType() const {
            for (const auto& obj : gameObjects) {
                if (!obj) continue;
                auto* component = obj->getComponent<T>();
                if (component) return component;
            }
            return nullptr;
        }

        template<typename T>
        std::vector<T*> Scene::FindComponentsOfType() const {
            std::vector<T*> result;
            for (const auto& obj : gameObjects) {
                if (!obj) continue;
                if (auto* component = obj->getComponent<T>()) {
                    result.push_back(component);
                }
            }
            return result;
        }
    }
}