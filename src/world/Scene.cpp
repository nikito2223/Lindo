#include "Scene.h"
#include <Component/GameObject/GameObject.h>
#include <algorithm>

namespace Lindo {
    namespace World {

        Scene::~Scene() {
            OnDestroy();
            gameObjects.clear();
            gameObjectMap.clear();
        }

        Lindo::World::GameObject* Scene::CreateGameObject(const std::string& name) {
            auto obj = std::make_unique<Lindo::World::GameObject>(name, sceneName);
            GameObject* ptr = obj.get();
            gameObjects.push_back(std::move(obj));
            gameObjectMap[name] = ptr;
            return ptr;
        }

        void Scene::AddGameObject(Lindo::World::GameObject* obj) {
            if (!obj) return;

            // Передаем владение сырым указателем вектору unique_ptr
            gameObjects.push_back(std::unique_ptr<Lindo::World::GameObject>(obj));
            gameObjectMap[obj->getName()] = obj;
        }

        void Scene::DestroyGameObject(Lindo::World::GameObject* obj) {
            if (!obj) return;

            auto it = std::find_if(gameObjects.begin(), gameObjects.end(),
                [obj](const std::unique_ptr<Lindo::World::GameObject>& ptr) { return ptr.get() == obj; });

            if (it != gameObjects.end()) {
                gameObjectMap.erase((*it)->getName());
                gameObjects.erase(it);
            }
        }

        int Scene::countAllGameObjectsRecursive() const {
            int count = static_cast<int>(gameObjects.size());
            for (const auto& obj : gameObjects) {
                count += obj->countChildrenRecursive();
            }
            return count;
        }

        void Scene::DestroyGameObject(const std::string& name) {
            auto it = gameObjectMap.find(name);
            if (it != gameObjectMap.end()) {
                DestroyGameObject(it->second);
            }
        }

        Lindo::World::GameObject* Scene::FindGameObject(const std::string& name) const {
            auto it = gameObjectMap.find(name);
            return it != gameObjectMap.end() ? it->second : nullptr;
        }

        std::vector<Lindo::World::GameObject*> Scene::FindGameObjectsByTag(const std::string& tag) const {
            std::vector<Lindo::World::GameObject*> result;
            for (const auto& obj : gameObjects) {
                if (obj->getTag() == tag) {
                    result.push_back(obj.get());
                }
            }
            return result;
        }
    }
}