#pragma once

#include "BoxCollider.h"
#include "SphereCollider.h"
#include "CapsuleCollider.h"
#include <vector>
#include <memory>
#include <functional>
#include <unordered_map>
#include <Graphics/core/Shader.h>

// Тип коллизии
struct CollisionEvent {
    Collider* colliderA;
    Collider* colliderB;
    CollisionInfo info;
    bool isEnter;     // true = вошли в коллизию, false = вышли из коллизии
    float timestamp;  // Время коллизии
};

// Callback для коллизий
using CollisionCallback = std::function<void(const CollisionEvent&)>;

class CollisionSystem {
public:
    // Singleton паттерн
    static CollisionSystem& getInstance() {
        static CollisionSystem instance;
        return instance;
    }

    // Добавление/удаление коллайдеров
    void addCollider(Collider* collider, const std::string& tag = "");
    void removeCollider(Collider* collider);
    void clearColliders();

    // Получение коллайдеров по тегу
    std::vector<Collider*> getCollidersByTag(const std::string& tag) const;

    // Проверка всех коллизий
    void update(float deltaTime);

    // Проверка конкретных коллизий
    bool checkCollision(Collider* a, Collider* b,
        CollisionInfo* info = nullptr) const;

    // Raycasting
    struct RaycastResult {
        bool hit = false;
        Collider* collider = nullptr;
        float distance = 0.0f;
        glm::vec3 point;
        glm::vec3 normal;
    };

    RaycastResult raycast(const glm::vec3& origin, const glm::vec3& direction,
        float maxDistance = 1000.0f,
        const std::string& tagFilter = "") const;

    // Регистрация callback'ов
    void registerCollisionCallback(const std::string& tagA, const std::string& tagB,
        CollisionCallback callback);
    void registerTriggerCallback(const std::string& tag, CollisionCallback callback);

    // Отладка
    void drawDebug(Shader& shader) const;
    void toggleDebugView() { debugEnabled = !debugEnabled; }
    void setDebugEnabled(bool enabled) { debugEnabled = enabled; }

    // Слои коллизий (опционально, для оптимизации)
    void setCollisionLayer(const std::string& tag, int layer);
    void setLayerCollision(int layerA, int layerB, bool canCollide);

private:
    CollisionSystem() = default;
    ~CollisionSystem() = default;
    CollisionSystem(const CollisionSystem&) = delete;
    CollisionSystem& operator=(const CollisionSystem&) = delete;

    // Внутренние структуры
    struct ColliderEntry {
        Collider* collider;
        std::string tag;
        int layer = 0;
        bool wasColliding = false;
    };

    // Коллайдеры и их теги
    std::vector<ColliderEntry> colliders;
    std::unordered_map<std::string, std::vector<CollisionCallback>> collisionCallbacks;
    std::unordered_map<std::string, std::vector<CollisionCallback>> triggerCallbacks;

    // Слои коллизий
    std::unordered_map<std::string, int> tagToLayer;
    std::unordered_map<int, std::unordered_map<int, bool>> layerCollisionMatrix;

    bool debugEnabled = true;

    // Внутренние методы
    void checkPairCollision(size_t i, size_t j);
    void fireCollisionEvent(const ColliderEntry& a, const ColliderEntry& b,
        const CollisionInfo& info, bool isEnter);
};