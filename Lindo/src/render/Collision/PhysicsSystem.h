// PhysicsSystem.h
#pragma once

#include <vector>
#include <memory>
#include "RigidBody.h"
#include "CollisionSystem.h"

class PhysicsSystem {
public:
    static PhysicsSystem& getInstance();

    void addRigidBody(std::shared_ptr<RigidBody> body, const std::string& tag = "");
    void removeRigidBody(std::shared_ptr<RigidBody> body);

    void update(float deltaTime);

    // Настройки
    void setGravity(const glm::vec3& gravity) { this->gravity = gravity; }
    void setIterations(int iterations) { this->iterations = iterations; }

private:
    PhysicsSystem() = default;
    ~PhysicsSystem() = default;

    struct RigidBodyEntry {
        std::shared_ptr<RigidBody> body;
        std::string tag;
    };

    std::vector<RigidBodyEntry> bodies;
    glm::vec3 gravity = glm::vec3(0.0f, -9.81f, 0.0f);
    int iterations = 10; // для разрешения коллизий

    void resolveCollision(std::shared_ptr<RigidBody> bodyA,
        std::shared_ptr<RigidBody> bodyB,
        const CollisionInfo& info);
};