// RigidBody.h
#pragma once
#include "render/Collision/Collider.h" // для типа Collider
#include <glm/glm.hpp>
#include "render/mesh/transform.h"

class RigidBody {
public:
    RigidBody(Transform* transform = nullptr, float mass = 1.0f);

    void applyForce(const glm::vec3& force);
    void applyImpulse(const glm::vec3& impulse);
    void integrate(float deltaTime, const glm::vec3& gravity);

    // Свойства
    float mass;
    float invMass;           // 1/mass (0 для статических объектов)
    glm::vec3 velocity;
    glm::vec3 acceleration;
    bool useGravity = true;
    float gravityScale = 1.0f;
    float restitution = 0.2f; // упругость
    float friction = 0.5f;    // трение (пока не используется)

    // Привязка к трансформации
    Transform* transform;
    std::shared_ptr<Collider> collider;

    // Флаги состояния
    bool isGrounded = false;

private:
    glm::vec3 forceAccumulator;
};