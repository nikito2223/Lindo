// RigidBody.cpp
#include "RigidBody.h"
#include <Component/GameObject/GameObject.h>
#include <iostream>

RigidBody::RigidBody(float mass)
    : mass(mass), invMass(mass > 0.0f ? 1.0f / mass : 0.0f),
    velocity(0.0f), acceleration(0.0f), forceAccumulator(0.0f) {}

void RigidBody::applyForce(const glm::vec3& force) {
    forceAccumulator += force;
}

void RigidBody::applyImpulse(const glm::vec3& impulse) {
    velocity += impulse * invMass;
}

void RigidBody::integrate(float deltaTime, const glm::vec3& gravity) {
    if (!owner || invMass == 0.0f) return;

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

    const float maxSpeed = 10.0f; // настройте под свои нужды
    float speed = glm::length(velocity);
    if (speed > maxSpeed) {
        velocity = glm::normalize(velocity) * maxSpeed;
    }
    
    owner->transform.position += velocity * deltaTime;
}
