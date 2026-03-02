// RigidBody.cpp
#include "RigidBody.h"
#include <iostream>

RigidBody::RigidBody(Transform* transform, float mass)
    : transform(transform), mass(mass), invMass(mass > 0.0f ? 1.0f / mass : 0.0f),
    velocity(0.0f), acceleration(0.0f), forceAccumulator(0.0f) {}

void RigidBody::applyForce(const glm::vec3& force) {
    forceAccumulator += force;
}

void RigidBody::applyImpulse(const glm::vec3& impulse) {
    velocity += impulse * invMass;
}

void RigidBody::integrate(float deltaTime, const glm::vec3& gravity) {
    if (invMass == 0.0f || !transform) return;

    if (useGravity) {
        velocity += gravity * gravityScale * deltaTime;
    }

    if (forceAccumulator != glm::vec3(0.0f)) {
        acceleration = forceAccumulator * invMass;
        velocity += acceleration * deltaTime;
        forceAccumulator = glm::vec3(0.0f);
    }

    // Демпфирование (чтобы гасить микроколебания)
    velocity *= 0.995f;

    const float maxSpeed = 8.0f; // настройте под свои нужды
    float speed = glm::length(velocity);
    if (speed > maxSpeed) {
        velocity = glm::normalize(velocity) * maxSpeed;
    }

    transform->position += velocity * deltaTime;
}
