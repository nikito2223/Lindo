#include "Component/GameObject/GameObject.h"
#include <iostream>

namespace Lindo {
    namespace World {

        void GameObject::printHierarchy(int indent) {
            std::string ind(indent, ' ');
            std::cout << ind << "GameObject: " << name
                << " (Tag: " << tag << ")"
                << " Active: " << (isActive ? "Yes" : "No")
                << " Components: " << components.size()
                << std::endl;

            for (auto* child : children) {
                child->printHierarchy(indent + 2);
            }
        }

        // ----- Жизненный цикл -----

        void GameObject::Update() {
            if (!isActive) return;

            if (!started) {
                started = true;
                // Использование индекса вместо range-based for предотвращает краш,
                // если OnStart создаст новый компонент во время итерации
                for (size_t i = 0; i < components.size(); ++i) {
                    components[i]->OnStart();
                }
            }

            for (size_t i = 0; i < components.size(); ++i) {
                components[i]->OnUpdate();
            }

            for (size_t i = 0; i < children.size(); ++i) {
                children[i]->Update();
            }
        }

        void GameObject::Draw(Lindo::Graphics::Shader& shader) {
            if (!isActive) return;

            for (auto& comp : components) {
                comp->OnDraw(shader);
            }
            for (auto* child : children) {
                child->Draw(shader);
            }
        }

        void GameObject::invalidateCache() {
            componentCache.clear();
        }

        // ----- Иерархия (Родитель / Дети) -----

        void GameObject::setParent(GameObject* newParent, bool keepWorldTransform) {
            if (parent == newParent) return;

            // Защита от циклического назначения (нельзя сделать родителям своего же ребенка)
            if (newParent && newParent->isChildOf(this)) {
                std::cout << "[GameObject] Error: Cannot set child as parent!" << std::endl;
                return;
            }

            glm::vec3 worldPos = getWorldPosition();

            if (parent) {
                parent->removeChild(this);
            }

            parent = newParent;

            if (parent) {
                parent->children.push_back(this);
            }

            // Пересчитываем локальные координаты относительно нового родителя
            if (keepWorldTransform) {
                if (parent) {
                    transform.position = worldPos - parent->getWorldPosition();
                }
                else {
                    transform.position = worldPos;
                }
            }
        }

        void GameObject::addChild(GameObject* child) {
            if (child) {
                child->setParent(this, true);
            }
        }

        void GameObject::removeChild(GameObject* child) {
            auto it = std::find(children.begin(), children.end(), child);
            if (it != children.end()) {
                child->parent = nullptr;
                children.erase(it);
            }
        }

        std::vector<GameObject*> GameObject::getAllChildren() {
            std::vector<GameObject*> result = children;
            for (auto* child : children) {
                auto grandChildren = child->getAllChildren();
                result.insert(result.end(), grandChildren.begin(), grandChildren.end());
            }
            return result;
        }

        // ----- Поиск объектов -----

        GameObject* GameObject::findChildByName(const std::string& childName) {
            for (auto* child : children) {
                if (child->name == childName) return child;
                auto* found = child->findChildByName(childName);
                if (found) return found;
            }
            return nullptr;
        }

        std::vector<GameObject*> GameObject::findChildrenByTag(const std::string& childTag) {
            std::vector<GameObject*> result;
            for (auto* child : children) {
                if (child->tag == childTag) result.push_back(child);
                auto found = child->findChildrenByTag(childTag);
                result.insert(result.end(), found.begin(), found.end());
            }
            return result;
        }

        void GameObject::setActive(bool active) {
            if (isActive == active) return;
            isActive = active;

            for (auto* child : children) {
                child->setActive(active);
            }
        }

        // ----- Трансформация в мировых координатах -----

        glm::vec3 GameObject::getWorldPosition() const {
            return glm::vec3(getWorldMatrix()[3]);
        }

        glm::mat4 GameObject::getWorldMatrix() const {
            if (parent) {
                return parent->getWorldMatrix() * transform.getMatrix();
            }
            return transform.getMatrix();
        }

    }
}