#pragma once

#include <vector>
#include <memory>
#include <Physics/RaycastHit.h>
#include <Physics/RigidBody.h>


class PhysicsSystem {
public:
    static PhysicsSystem& getInstance();

    void addRigidBody(std::shared_ptr<RigidBody> body, const std::string& tag = "");
    void removeRigidBody(std::shared_ptr<RigidBody> body);

    void update(float deltaTime);

    // Ќастройки
    void setGravity(const glm::vec3& gravity) { this->gravity = gravity; }
    void setIterations(int iterations) { this->iterations = iterations; }

    bool raycast(const glm::vec3& origin,
        const glm::vec3& direction,
        float maxDistance,
        RaycastHit& hit,
        const std::string& tagFilter = "",
        std::shared_ptr<RigidBody> ignoreBody = nullptr) const;

    // ¬озвращает все пересечени€, отсортированные по возрастанию рассто€ни€.
    std::vector<RaycastHit> raycastAll(const glm::vec3& origin,
        const glm::vec3& direction,
        float maxDistance,
        const std::string& tagFilter = "",
        std::shared_ptr<RigidBody> ignoreBody = nullptr) const;

private:
    PhysicsSystem() = default;
    ~PhysicsSystem() = default;

    struct RigidBodyEntry {
        std::shared_ptr<RigidBody> body;
        std::string tag;
    };

    std::vector<RigidBodyEntry> bodies;
    glm::vec3 gravity = glm::vec3(0.0f, -9.81f, 0.0f);
    int iterations = 10; // дл€ разрешени€ коллизий

    void resolveCollision(std::shared_ptr<RigidBody> bodyA,
        std::shared_ptr<RigidBody> bodyB,
        const CollisionInfo& info);
};