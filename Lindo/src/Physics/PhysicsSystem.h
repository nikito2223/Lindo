#pragma once

#include <string>
#include <glm/glm.hpp>
#include <memory>
#include <vector>
//#include "Physics/Collider/ColliderType.h"
#include "Physics/RaycastHit.h"
#include "Physics/RigidBody.h"

class CollisionInfo;

class PhysicsSystem {
public:
    static PhysicsSystem& getInstance();

    void addRigidBody(RigidBody* body, const std::string& tag = "");
    void removeRigidBody(RigidBody* body);

    void update(float deltaTime);

    // Настройки
    void setGravity(const glm::vec3& gravity) { this->gravity = gravity; }
    glm::vec3& getGravity() { return gravity; }
    void setIterations(int iterations) { this->iterations = iterations; }

    bool raycast(const glm::vec3& origin,
        const glm::vec3& direction,
        float maxDistance,
        RaycastHit& hit,
        const std::string& tagFilter = "",
        RigidBody* ignoreBody = nullptr,
        bool ignoreTriggers = true) const; // Добавили этот флаг

    // Возвращает все пересечения, отсортированные по возрастанию расстояния.
    std::vector<RaycastHit> raycastAll(const glm::vec3& origin,
        const glm::vec3& direction,
        float maxDistance,
        const std::string& tagFilter = "",
        RigidBody* ignoreBody = nullptr) const;

private:
    PhysicsSystem() = default;
    ~PhysicsSystem() = default;

    struct RigidBodyEntry {
        RigidBody* body;
        std::string tag;
    };

    std::vector<RigidBodyEntry> bodies;
    glm::vec3 gravity = glm::vec3(0.0f, -9.81f, 0.0f);
    int iterations = 10; // для разрешения коллизий

    void resolveCollision(RigidBody* bodyA, RigidBody* bodyB, const CollisionInfo& info);
};