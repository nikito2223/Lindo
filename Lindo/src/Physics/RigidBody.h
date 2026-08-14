#pragma once

#include <glm/glm.hpp>
#include <memory>
#include <Component/Component.h>

class GameObject;
class Collider;

class RigidBody : public Component {
public:
    RigidBody(float mass = 1.0f);

    void applyForce(const glm::vec3& force);
    void applyImpulse(const glm::vec3& impulse);
    void integrate(float deltaTime, const glm::vec3& gravity);

    float mass;
    float invMass;
    glm::vec3 velocity;
    glm::vec3 acceleration;
    bool useGravity = true;
    float gravityScale = 1.0f;
    float restitution = 0.2f;
    float friction = 0.5f;

    Collider* collider = nullptr;

    bool isGrounded = true;

private:
    glm::vec3 forceAccumulator;
};