#include "Component/GameObject/GameObject.h"
#include <iostream>
#include <exception>

namespace Lindo {
    namespace World {

        /**
         * @brief Рекурсивно форматирует и выводит структуру иерархии через логгер.
         * @param indent Количество пробелов для отступа.
         */
        void GameObject::printHierarchy(int indent) {
            std::string ind(indent, ' ');
            LOG_INFO(ind + "GameObject: " + name
                + " (Tag: " + tag + ")"
                + " Active: " + (isActive ? "Yes" : "No")
                + " Components: " + std::to_string(components.size()));

            for (auto* child : children) {
                if (child) {
                    child->printHierarchy(indent + 2);
                }
            }
        }

        /**
         * @brief Вызывает итерацию обновления компонентов и дочерних сущностей.
         */
        void GameObject::Update() {
            if (!isActive) return;

            if (!started) {
                started = true;
                for (size_t i = 0; i < components.size(); ++i) {
                    components[i]->OnStart();
                }
            }

            for (size_t i = 0; i < components.size(); ++i) {
                components[i]->OnUpdate();
            }

            for (size_t i = 0; i < children.size(); ++i) {
                if (children[i]) {
                    children[i]->Update();
                }
            }
        }

        /**
         * @brief Вызывает отрисовку у прикрепленных компонентов и отправляет вызов дочерним объектам.
         * @param shader Активный шейдер рендеринга.
         */
        void GameObject::Draw(Lindo::Graphics::Shader& shader) {
            if (!isActive) return;

            for (auto& comp : components) {
                if (comp) {
                    comp->OnDraw(shader);
                }
            }
            for (auto* child : children) {
                if (child) {
                    child->Draw(shader);
                }
            }
        }

        /**
         * @brief Отвечает за отрисовку отладочных гизмо компонентов объекта и дочерних сущностей с безопасной обработкой исключений.
         */
        void GameObject::DrawGizmos() {
            if (!isActive) return;

            try {
                for (size_t i = 0; i < components.size(); ++i) {
                    const auto& comp = components[i];
                    if (!comp) {
                        LOG_WARN("[GameObject] Object '" + name + "' contains nullptr component at index " + std::to_string(i));
                        continue;
                    }

                    try {
                        comp->OnDrawGizmos();
                    }
                    catch (const std::exception& e) {
                        LOG_ERROR("[GameObject] Exception in OnDrawGizmos for component (" +
                            std::string(typeid(*comp).name()) + ") on GameObject '" + name + "': " + e.what());
                    }
                    catch (...) {
                        LOG_CRITICAL("[GameObject] Unknown critical crash in OnDrawGizmos for component (" +
                            std::string(typeid(*comp).name()) + ") on GameObject '" + name + "'");
                    }
                }

                for (auto* child : children) {
                    if (child) {
                        child->DrawGizmos();
                    }
                }
            }
            catch (const std::exception& e) {
                LOG_ERROR("[GameObject] Error DrawGizmos\n info: " + std::string(e.what()));
            }
        }

        /**
         * @brief Сбрасывает кэш компонентов по типам.
         */
        void GameObject::invalidateCache() {
            componentCache.clear();
        }

        /**
         * @brief Привязывает объект к новому родителю с перерасчетом трансформации при необходимости.
         * @param newParent Новый родительский объект.
         * @param keepWorldTransform Флаг сохранения мировой позиции объекта при смене родителя.
         */
        void GameObject::setParent(GameObject* newParent, bool keepWorldTransform) {
            if (parent == newParent) return;

            if (newParent == this) {
                LOG_ERROR("[GameObject] Cannot set object '" + name + "' as its own parent!");
                return;
            }

            if (newParent && newParent->isChildOf(this)) {
                LOG_ERROR("[GameObject] Error: Cannot set child '" + newParent->name + "' as parent for '" + name + "'!");
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

            if (keepWorldTransform) {
                if (parent) {
                    transform.position = worldPos - parent->getWorldPosition();
                }
                else {
                    transform.position = worldPos;
                }
            }
        }

        /**
         * @brief Добавляет дочерний объект.
         * @param child Дочерний объект.
         */
        void GameObject::addChild(GameObject* child) {
            if (!child) {
                LOG_WARN("[GameObject] Attempted to add nullptr child to '" + name + "'");
                return;
            }
            child->setParent(this, true);
        }

        /**
         * @brief Исключает дочерний объект из списка детей.
         * @param child Удаляемый дочерний объект.
         */
        void GameObject::removeChild(GameObject* child) {
            if (!child) {
                LOG_WARN("[GameObject] Attempted to remove nullptr child from '" + name + "'");
                return;
            }

            auto it = std::find(children.begin(), children.end(), child);
            if (it != children.end()) {
                child->parent = nullptr;
                children.erase(it);
            }
            else {
                LOG_WARN("[GameObject] Child '" + child->name + "' not found in '" + name + "' during removeChild()");
            }
        }

        /**
         * @brief Собирает все дочерние объекты рекурсивно.
         * @return Вектор с дочерними элементами.
         */
        std::vector<GameObject*> GameObject::getAllChildren() {
            std::vector<GameObject*> result = children;
            for (auto* child : children) {
                if (child) {
                    auto grandChildren = child->getAllChildren();
                    result.insert(result.end(), grandChildren.begin(), grandChildren.end());
                }
            }
            return result;
        }

        /**
         * @brief Осуществляет поиск ребенка по названию.
         * @param childName Имя объекта.
         * @return Найденный объект или nullptr.
         */
        GameObject* GameObject::findChildByName(const std::string& childName) {
            for (auto* child : children) {
                if (!child) continue;
                if (child->name == childName) return child;
                auto* found = child->findChildByName(childName);
                if (found) return found;
            }
            return nullptr;
        }

        /**
         * @brief Осуществляет поиск всех детей по тегу.
         * @param childTag Тег объектов.
         * @return Вектор найденных дочерних элементов.
         */
        std::vector<GameObject*> GameObject::findChildrenByTag(const std::string& childTag) {
            std::vector<GameObject*> result;
            for (auto* child : children) {
                if (!child) continue;
                if (child->tag == childTag) result.push_back(child);
                auto found = child->findChildrenByTag(childTag);
                result.insert(result.end(), found.begin(), found.end());
            }
            return result;
        }

        /**
         * @brief Переключает активность текущей сущности и ее дочерних ветвей.
         * @param active Флаг активности.
         */
        void GameObject::setActive(bool active) {
            if (isActive == active) return;
            isActive = active;

            for (auto* child : children) {
                if (child) {
                    child->setActive(active);
                }
            }
        }

        /**
         * @brief Вычисляет абсолютную мировую позицию объекта в пространстве.
         * @return Вектор координат в мировом пространстве.
         */
        glm::vec3 GameObject::getWorldPosition() const {
            return glm::vec3(getWorldMatrix()[3]);
        }

        void GameObject::setWorldPosition(const glm::vec3& position) {
            if (!parent) {
                transform.setLocalPosition(position);
                return;
            }

            const glm::mat4 parentInverse = glm::inverse(parent->getWorldMatrix());
            transform.setLocalPosition(glm::vec3(parentInverse * glm::vec4(position, 1.0f)));
        }

        /**
         * @brief Строит мировую матрицу трансформации объекта с учетом всей иерархии предков.
         * @return Мировая матрица модели.
         */
        glm::mat4 GameObject::getWorldMatrix() const {
            if (parent) {
                return parent->getWorldMatrix() * transform.getMatrix();
            }
            return transform.getMatrix();
        }

    }
}