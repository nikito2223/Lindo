#pragma once

#include <string>
#include <memory>
#include <vector>
#include <algorithm>
#include <typeindex>
#include <unordered_map>
#include <stdexcept>
#include "Component/Component.h"
#include "Component/GameObject/transform.h"

namespace Lindo {
    namespace Graphics {
        class Shader;
    }

    namespace World {

        /// <summary>
        /// Базовый класс сущности на сцене. Содержит компоненты, трансформ и связи с дочерними объектами.
        /// </summary>
        class GameObject {
        public:
            /// <summary> Имя объекта на сцене. </summary>
            std::string name;

            /// <summary> Тег объекта для группировки и поиска (например, "Player", "Enemy"). </summary>
            std::string tag;

            /// <summary> Локальный трансформ объекта (позиция, поворот, масштаб). </summary>
            Lindo::Math::Transform transform;

            /// <summary> Владеющий список всех компонентов объекта. </summary>
            std::vector<std::unique_ptr<Component>> components;

            /// <summary> Быстрый кэш для доступа к компонентам по их типам. </summary>
            std::unordered_map<std::type_index, Component*> componentCache;

            /// <summary> Флаг активности объекта. Неактивные объекты не обновляются и не рендерятся. </summary>
            bool isActive = true;

            /// <summary> Флаг статичности объекта (для оптимизации физики и батчинга). </summary>
            bool isStatic = false;

            /// <summary> Флаг сохранения объекта при смене сцены (аналог DontDestroyOnLoad). </summary>
            bool isPersistent = false;

            /// <summary> Флаг отбрасывания теней объектом. </summary>
            bool castsShadows = true;

            /// <summary> Флаг прохождения инициализации Start. </summary>
            bool started = false;

            /// <summary> Список указателей на дочерние объекты. </summary>
            std::vector<GameObject*> children;

            /// <summary> Указатель на родительский объект (nullptr, если объект корневой). </summary>
            GameObject* parent = nullptr;

            /// <summary>
            /// Конструктор игрового объекта.
            /// </summary>
            /// <param name="name">Имя объекта.</param>
            /// <param name="tag">Тег объекта.</param>
            GameObject(std::string name = "NewEntity", std::string tag = "Untagged")
                : name(name), tag(tag) {
            }

            virtual ~GameObject() = default;

            /// <summary> Возвращает имя объекта. </summary>
            const std::string& getName() const { return name; }

            /// <summary> Устанавливает новое имя объекта. </summary>
            void setName(const std::string& newName) { name = newName; }

            /// <summary> Возвращает тег объекта. </summary>
            const std::string& getTag() const { return tag; }

            /// <summary> Устанавливает новый тег объекта. </summary>
            void setTag(const std::string& newTag) { tag = newTag; }

            // ----- Работа с компонентами -----

            /// <summary>
            /// Создает и добавляет новый компонент на объект.
            /// </summary>
            /// <typeparam name="T">Тип компонента, унаследованный от Component.</typeparam>
            /// <param name="args">Аргументы для конструктора компонента.</param>
            /// <returns>Указатель на созданный компонент.</returns>
            template<typename T, typename... Args>
            T* addComponent(Args&&... args) {
                static_assert(std::is_base_of<Component, T>::value,
                    "T must be derived from Component");

                auto comp = std::make_unique<T>(std::forward<Args>(args)...);
                comp->gameObject = this;
                T* ptr = comp.get();

                componentCache[std::type_index(typeid(T))] = ptr;
                components.push_back(std::move(comp));
                return ptr;
            }

            /// <summary>
            /// Находит и возвращает первый компонент указанного типа.
            /// </summary>
            /// <typeparam name="T">Тип компонента.</typeparam>
            /// <returns>Указатель на компонент или nullptr, если компонент не найден.</returns>
            template<typename T>
            T* getComponent() {
                auto it = componentCache.find(std::type_index(typeid(T)));
                if (it != componentCache.end()) {
                    return dynamic_cast<T*>(it->second);
                }

                for (auto& comp : components) {
                    if (auto casted = dynamic_cast<T*>(comp.get())) {
                        componentCache[std::type_index(typeid(T))] = casted;
                        return casted;
                    }
                }
                return nullptr;
            }

            /// <summary>
            /// Находит все компоненты указанного типа на объекте.
            /// </summary>
            /// <typeparam name="T">Тип компонента.</typeparam>
            /// <returns>Вектор указателей на найденные компоненты.</returns>
            template<typename T>
            std::vector<T*> getComponents() {
                std::vector<T*> result;
                for (auto& comp : components) {
                    if (auto casted = dynamic_cast<T*>(comp.get())) {
                        result.push_back(casted);
                    }
                }
                return result;
            }

            /// <summary>
            /// Находит все компоненты, реализующие указанный интерфейс или абстрактный класс.
            /// </summary>
            /// <typeparam name="Interface">Тип интерфейса.</typeparam>
            /// <returns>Вектор указателей на компоненты, приведённые к интерфейсу.</returns>
            template<typename Interface>
            std::vector<Interface*> getInterfaces() {
                std::vector<Interface*> result;
                for (auto& comp : components) {
                    if (auto casted = dynamic_cast<Interface*>(comp.get())) {
                        result.push_back(casted);
                    }
                }
                return result;
            }

            /// <summary>
            /// Удаляет первый компонент указанного типа.
            /// </summary>
            /// <typeparam name="T">Тип компонента для удаления.</typeparam>
            /// <returns>true, если компонент был найден и удален.</returns>
            template<typename T>
            bool removeComponent() {
                auto it = std::find_if(components.begin(), components.end(),
                    [](const std::unique_ptr<Component>& comp) {
                        return dynamic_cast<T*>(comp.get()) != nullptr;
                    });

                if (it != components.end()) {
                    (*it)->OnDestroy();
                    components.erase(it);
                    invalidateCache(); // Сброс кэша для предотвращения висячих указателей
                    return true;
                }
                return false;
            }

            /// <summary>
            /// Проверяет, прикреплен ли компонент указанного типа к объекту.
            /// </summary>
            /// <typeparam name="T">Тип компонента.</typeparam>
            template<typename T>
            bool hasComponent() {
                return getComponent<T>() != nullptr;
            }

            /// <summary>
            /// Возвращает существующий компонент или создает новый, если он отсутствует.
            /// </summary>
            /// <typeparam name="T">Тип компонента.</typeparam>
            /// <param name="args">Аргументы конструктора, если компонент придется создавать.</param>
            template<typename T, typename... Args>
            T* getOrAddComponent(Args&&... args) {
                T* comp = getComponent<T>();
                if (!comp) {
                    comp = addComponent<T>(std::forward<Args>(args)...);
                }
                return comp;
            }

            /// <summary>
            /// Возвращает обязательный компонент. Если он отсутствует, выбрасывает исключение.
            /// </summary>
            /// <typeparam name="T">Тип компонента.</typeparam>
            /// <exception std::runtime_error>Выбрасывается, если компонент не найден.</exception>
            template<typename T>
            T* reqComponent() {
                T* comp = getComponent<T>();
                if (!comp) {
                    throw std::runtime_error("Required component missing on GameObject: " + name);
                }
                return comp;
            }

            void setParent(GameObject* newParent, bool keepWorldTransform = true);

            GameObject* getRoot() {
                GameObject* current = this;
                while (current->parent != nullptr) {
                    current = current->parent;
                }
                return current;
            }

            bool isChildOf(const GameObject* potentialParent) const {
                const GameObject* current = parent;
                while (current != nullptr) {
                    if (current == potentialParent) return true;
                    current = current->parent;
                }
                return false;
            }

            // Поиск компонентов по иерархии вверх
            template<typename T>
            T* getComponentInParent() {
                GameObject* current = this;
                while (current != nullptr) {
                    T* comp = current->getComponent<T>();
                    if (comp) return comp;
                    current = current->parent;
                }
                return nullptr;
            }

            // Поиск компонентов по иерархии вниз (включая себя)
            template<typename T>
            T* getComponentInChildren() {
                T* comp = getComponent<T>();
                if (comp) return comp;

                for (auto* child : children) {
                    T* childComp = child->getComponentInChildren<T>();
                    if (childComp) return childComp;
                }
                return nullptr;
            }

            template<typename T>
            void getComponentsInChildren(std::vector<T*>& outList) {
                auto selfComps = getComponents<T>();
                outList.insert(outList.end(), selfComps.begin(), selfComps.end());

                for (auto* child : children) {
                    child->getComponentsInChildren<T>(outList);
                }
            }

            // ----- Иерархия (Дочерние / Родительские объекты) -----

            /// <summary> Добавляет дочерний объект. </summary>
            void addChild(GameObject* child);

            /// <summary> Удаляет дочерний объект из списка потомков. </summary>
            void removeChild(GameObject* child);

            /// <summary> Возвращает полный вектор всех дочерних объектов рекурсивно. </summary>
            std::vector<GameObject*> getAllChildren();

            /// <summary> Поиск дочернего объекта по имени. </summary>
            GameObject* findChildByName(const std::string& childName);

            /// <summary> Поиск дочерних объектов по тегу. </summary>
            std::vector<GameObject*> findChildrenByTag(const std::string& childTag);

            // ----- Состояние и Трансформация -----

            /// <summary> Переключает активность объекта и всех его дочерних элементов. </summary>
            void setActive(bool active);

            /// <summary> Рассчитывает абсолютную мировую позицию с учетом родительских трансформаций. </summary>
            glm::vec3 getWorldPosition() const;

            /// <summary> Рассчитывает итоговую мировую матрицу трансформации объекта. </summary>
            glm::mat4 getWorldMatrix() const;

            // ----- Жизненный цикл и Рендеринг -----

            /// <summary> Обновляет логику всех прикрепленных компонентов и дочерних объектов. </summary>
            virtual void Update();

            /// <summary> Отрисовывает геометрию компонентов объекта. </summary>
            virtual void Draw(Lindo::Graphics::Shader& shader);

            /// <summary> Полностью очищает кэш компонентов. </summary>
            void invalidateCache();

            /// <summary> Выводит иерархию объекта и его детей в консоль для отладки. </summary>
            void printHierarchy(int indent = 0);
        };
    }
}