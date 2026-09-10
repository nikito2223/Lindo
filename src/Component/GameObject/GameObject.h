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
#include <world/managers/LayerManager.h>

namespace Lindo {
    namespace Graphics {
        class Shader;
    }

    namespace World {

        /**
         * @brief Базовый класс сущности сцены. Содержит компоненты, трансформ и связи с дочерними объектами.
         */
        class GameObject {

        public:
            std::string name; 
            
        private:
            std::string tag = "Untagged";    ///< Хранимый тег объекта
            uint8_t layer = 0;               ///< Индекс слоя объекта (0..31)

        public:
            Lindo::Math::Transform transform;

            std::vector<std::unique_ptr<Component>> components;
            std::unordered_map<std::type_index, Component*> componentCache;

            bool isActive = true;
            bool isStatic = false;
            bool isPersistent = false;
            bool castsShadows = true;
            bool started = false;

            std::vector<GameObject*> children;
            GameObject* parent = nullptr;

            GameObject(std::string name = "NewEntity", std::string tag = "Untagged", uint8_t layer = 0)
                : name(name) {
                setTag(tag);
                setLayer(layer);
            }

            virtual ~GameObject() = default;

            /**
             * @brief Получает имя объекта.
             * @return Константная ссылка на имя.
             */
            const std::string& getName() const { return name; }

            /**
             * @brief Задает новое имя объекта.
             * @param newName Новое имя.
             */
            void setName(const std::string& newName) { name = newName; }

            /**
             * @brief Получает тег объекта.
             * @return Константная ссылка на тег.
             */
            const std::string& getTag() const { return tag; }
            
            void setTag(const std::string& newTag) {
                auto& lm = LayerManager::get();
                lm.registerTag(newTag); // Автоматическая регистрация при динамическом назначении
                tag = newTag;
            }

            bool compareTag(const std::string& otherTag) const {
                return tag == otherTag;
            }

            uint8_t getLayer() const { return layer; }
            
            void setLayer(uint8_t newLayer) {
                if (newLayer >= LayerManager::MAX_LAYERS) {
                    LOG_WARN("[GameObject] Invalid layer index: " + std::to_string(newLayer));
                    return;
                }
                layer = newLayer;
            }

            void setLayerByName(const std::string& layerName) {
                layer = LayerManager::get().getLayerByName(layerName);
            }

            std::string getLayerName() const {
                return LayerManager::get().getLayerName(layer);
            }

            LayerMask getLayerMask() const {
                return (1 << layer);
            }
            /**
             * @brief Создает и добавляет новый компонент на объект.
             * @tparam T Тип компонента, унаследованный от Component.
             * @tparam Args Типы аргументов конструктора компонента.
             * @param args Аргументы для конструктора компонента.
             * @return Указатель на созданный компонент.
             */
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

            /**
             * @brief Находит и возвращает первый компонент указанного типа.
             * @tparam T Тип компонента.
             * @return Указатель на компонент или nullptr, если компонент не найден.
             */
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

            /**
             * @brief Находит все компоненты указанного типа на объекте.
             * @tparam T Тип компонента.
             * @return Вектор указателей на найденные компоненты.
             */
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

            /**
             * @brief Находит все компоненты, реализующие указанный интерфейс или абстрактный класс.
             * @tparam Interface Тип интерфейса.
             * @return Вектор указателей на компоненты, приведённые к интерфейсу.
             */
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

            /**
             * @brief Удаляет первый компонент указанного типа.
             * @tparam T Тип компонента для удаления.
             * @return true, если компонент был найден и удален.
             */
            template<typename T>
            bool removeComponent() {
                auto it = std::find_if(components.begin(), components.end(),
                    [](const std::unique_ptr<Component>& comp) {
                        return dynamic_cast<T*>(comp.get()) != nullptr;
                    });

                if (it != components.end()) {
                    (*it)->OnDestroy();
                    components.erase(it);
                    invalidateCache();
                    return true;
                }
                return false;
            }

            /**
             * @brief Проверяет, прикреплен ли компонент указанного типа к объекту.
             * @tparam T Тип компонента.
             * @return true, если компонент существует.
             */
            template<typename T>
            bool hasComponent() {
                return getComponent<T>() != nullptr;
            }

            /**
             * @brief Возвращает существующий компонент или создает новый, если он отсутствует.
             * @tparam T Тип компонента.
             * @tparam Args Типы аргументов конструктора.
             * @param args Аргументы конструктора для создания нового компонента.
             * @return Указатель на компонент.
             */
            template<typename T, typename... Args>
            T* getOrAddComponent(Args&&... args) {
                T* comp = getComponent<T>();
                if (!comp) {
                    comp = addComponent<T>(std::forward<Args>(args)...);
                }
                return comp;
            }

            /**
             * @brief Возвращает обязательный компонент. Выбрасывает исключение, если он отсутствует.
             * @tparam T Тип компонента.
             * @return Указатель на компонент.
             * @throws std::runtime_error Выбрасывается, если компонент не найден.
             */
            template<typename T>
            T* reqComponent() {
                T* comp = getComponent<T>();
                if (!comp) {
                    throw std::runtime_error("Required component missing on GameObject: " + name);
                }
                return comp;
            }

            /**
             * @brief Устанавливает нового родителя для объекта.
             * @param newParent Указатель на нового родителя.
             * @param keepWorldTransform Сохранять ли мировые координаты объекта.
             */
            void setParent(GameObject* newParent, bool keepWorldTransform = true);

            /**
             * @brief Находит корневой объект иерархии.
             * @return Указатель на верхний родительский объект.
             */
            GameObject* getRoot() {
                GameObject* current = this;
                while (current->parent != nullptr) {
                    current = current->parent;
                }
                return current;
            }

            /**
             * @brief Проверяет, является ли объект потомком другого объекта.
             * @param potentialParent Предполагаемый родитель.
             * @return true, если объект находится внутри иерархии potentialParent.
             */
            bool isChildOf(const GameObject* potentialParent) const {
                const GameObject* current = parent;
                while (current != nullptr) {
                    if (current == potentialParent) return true;
                    current = current->parent;
                }
                return false;
            }

            /**
             * @brief Рекурсивный поиск компонента вверх по иерархии родителей.
             * @tparam T Тип компонента.
             * @return Указатель на найденный компонент или nullptr.
             */
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

            /**
             * @brief Рекурсивный поиск первого подходящего компонента вниз по иерархии детей.
             * @tparam T Тип компонента.
             * @return Указатель на найденный компонент или nullptr.
             */
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

            /**
             * @brief Заполняет вектор всеми компонентами типа T, найденными у объекта и его детей.
             * @tparam T Тип компонента.
             * @param outList Входной вектор для сохранения результатов.
             */
            template<typename T>
            void getComponentsInChildren(std::vector<T*>& outList) {
                auto selfComps = getComponents<T>();
                outList.insert(outList.end(), selfComps.begin(), selfComps.end());

                for (auto* child : children) {
                    child->getComponentsInChildren<T>(outList);
                }
            }

            /**
             * @brief Добавляет дочерний объект.
             * @param child Указатель на добавляемый объект.
             */
            void addChild(GameObject* child);

            /**
             * @brief Удаляет дочерний объект из списка потомков.
             * @param child Указатель на удаляемый объект.
             */
            void removeChild(GameObject* child);

            /**
             * @brief Возвращает полный список всех дочерних объектов рекурсивно.
             * @return Вектор указателей на все объекты-потомки.
             */
            std::vector<GameObject*> getAllChildren();

            /**
             * @brief Ищет дочерний объект по имени.
             * @param childName Имя искомого объекта.
             * @return Указатель на объект или nullptr.
             */
            GameObject* findChildByName(const std::string& childName);

            /**
             * @brief Ищет дочерние объекты по тегу.
             * @param childTag Тег искомых объектов.
             * @return Вектор найденных объектов.
             */
            std::vector<GameObject*> findChildrenByTag(const std::string& childTag);

            /**
             * @brief Переключает активность объекта и всех его дочерних элементов.
             * @param active Новое состояние активности.
             */
            void setActive(bool active);

            /**
             * @brief Рассчитывает абсолютную мировую позицию с учетом родительских трансформаций.
             * @return Вектор мировых координат glm::vec3.
             */
            glm::vec3 getWorldPosition() const;
            glm::mat4 getLocalMatrix() const { return transform.getLocalMatrix(); }
            glm::vec3 getLocalPosition() const { return transform.getLocalPosition(); }
            void setLocalPosition(const glm::vec3& position) { transform.setLocalPosition(position); }
            void setWorldPosition(const glm::vec3& position);

            /**
             * @brief Рассчитывает итоговую мировую матрицу трансформации объекта.
             * @return Матрица мировой трансформации glm::mat4.
             */
            glm::mat4 getWorldMatrix() const;

            /**
             * @brief Обновляет логику всех прикрепленных компонентов и дочерних объектов.
             */
            virtual void Update();

            /**
             * @brief Отрисовывает геометрию компонентов объекта.
             * @param shader Активный шейдер рендеринга.
             */
            virtual void Draw(Lindo::Graphics::Shader& shader);

            /**
             * @brief Вызывает отрисовку отладочной графики для всех компонентов и дочерних объектов.
             */
            virtual void DrawGizmos();

            /**
             * @brief Подсчитывает общее количество потомков в дереве иерархии.
             * @return Количество дочерних объектов.
             */
            int countChildrenRecursive() const {
                int count = static_cast<int>(children.size());
                for (const auto& child : children) {
                    count += child->countChildrenRecursive();
                }
                return count;
            }

            /**
             * @brief Полностью очищает кэш компонентов.
             */
            void invalidateCache();

            /**
             * @brief Выводит дерево иерархии объекта и его потомков в консоль.
             * @param indent Количество пробелов для отступа уровня иерархии.
             */
            void printHierarchy(int indent = 0);
        };
    }
}