#pragma once
#include <string>
#include <memory>
#include <vector>
#include <algorithm>
#include <typeindex>
#include <unordered_map>
#include <stdexcept>
#include <Objects/transform.h>
#include <Component/Component.h>

class Component;
class Shader;

class GameObject {
public:
    std::string name;
    std::string tag;
    Transform transform;
    std::vector<std::unique_ptr<Component>> components;

    // Для быстрого доступа по типу (кэш)
    std::unordered_map<std::type_index, Component*> componentCache;

    // Флаги состояния объекта
    bool isActive = true;
    bool isStatic = false;
    bool isPersistent = false;  // не удалять при смене сцены
    bool castsShadows = true;
    bool started = false;

    GameObject(std::string name = "NewEntity",
        std::string tag = "Untagged")
        : name(name), tag(tag) {}

    virtual ~GameObject() {
        // Очистка не нужна, unique_ptr сам всё удалит
    }

    // ----- Добавление компонентов -----
    template<typename T, typename... Args>
    T* addComponent(Args&&... args) {
        static_assert(std::is_base_of<Component, T>::value,
            "T must be derived from Component");

        auto comp = std::make_unique<T>(std::forward<Args>(args)...);
        comp->owner = this;
        T* ptr = comp.get();

        // Добавляем в кэш
        componentCache[std::type_index(typeid(T))] = ptr;

        components.push_back(std::move(comp));
        return ptr;
    }

    // ----- Получение одного компонента -----
    template<typename T>
    T* getComponent() {
        // Сначала проверяем кэш
        auto it = componentCache.find(std::type_index(typeid(T)));
        if (it != componentCache.end()) {
            return dynamic_cast<T*>(it->second);
        }

        // Если не нашли в кэше, ищем в векторе
        for (auto& comp : components) {
            if (auto casted = dynamic_cast<T*>(comp.get())) {
                // Обновляем кэш
                componentCache[std::type_index(typeid(T))] = casted;
                return casted;
            }
        }
        return nullptr;
    }

    // ----- ПОЛУЧЕНИЕ ВСЕХ КОМПОНЕНТОВ ОПРЕДЕЛЕННОГО ТИПА (НОВИНКА!) -----
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

    // ----- ПОЛУЧЕНИЕ ВСЕХ КОМПОНЕНТОВ, РЕАЛИЗУЮЩИХ ИНТЕРФЕЙС (НОВИНКА!) -----
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

    // ----- Удаление компонента (НОВИНКА!) -----
    template<typename T>
    bool removeComponent() {
        auto it = std::find_if(components.begin(), components.end(),
            [](const std::unique_ptr<Component>& comp) {
                return dynamic_cast<T*>(comp.get()) != nullptr;
            });

        if (it != components.end()) {
            // Удаляем из кэша
            componentCache.erase(std::type_index(typeid(T)));
            components.erase(it);
            return true;
        }
        return false;
    }

    // ----- Проверка наличия компонента (НОВИНКА!) -----
    template<typename T>
    bool hasComponent() {
        return getComponent<T>() != nullptr;
    }

    // ----- Получение компонента с созданием, если не существует (НОВИНКА!) -----
    template<typename T, typename... Args>
    T* getOrAddComponent(Args&&... args) {
        T* comp = getComponent<T>();
        if (!comp) {
            comp = addComponent<T>(std::forward<Args>(args)...);
        }
        return comp;
    }

    // ----- Работа с дочерними объектами (НОВИНКА!) -----
    std::vector<GameObject*> children;
    GameObject* parent = nullptr;

    void addChild(GameObject* child);

    void removeChild(GameObject* child);

    // Получить всех потомков (рекурсивно)
    std::vector<GameObject*> getAllChildren();

    // ----- Поиск объектов (НОВИНКА!) -----
    GameObject* findChildByName(const std::string& childName);

    std::vector<GameObject*> findChildrenByTag(const std::string& childTag);

    // ----- Управление активностью (НОВИНКА!) -----
    void setActive(bool active);

    // ----- Мировые координаты с учетом родителя (НОВИНКА!) -----
    glm::vec3 getWorldPosition() const;

    glm::mat4 getWorldMatrix() const;

    // ----- Шорткаты для компонентов (НОВИНКА!) -----
    template<typename T>
    T* reqComponent() {
        T* comp = getComponent<T>();
        if (!comp) {
            throw std::runtime_error("Required component " + std::string(typeid(T).name()) + " missing!");
        }
        return comp;
    }

    // ----- Отладка (НОВИНКА!) -----
    void printHierarchy(int indent = 0);

    // ----- Обновление и рендеринг -----
    virtual void Update(float deltaTime, const glm::vec3& gravity);

    virtual void Draw(Shader& shader);

    // ----- Очистка кэша (если компонент был удален напрямую) -----
    void invalidateCache();
};