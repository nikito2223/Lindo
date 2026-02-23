// PhysicsSystem.cpp
#include "PhysicsSystem.h"
#include <algorithm>

PhysicsSystem& PhysicsSystem::getInstance() {
    static PhysicsSystem instance;
    return instance;
}

void PhysicsSystem::addRigidBody(std::shared_ptr<RigidBody> body, const std::string& tag) {
    bodies.push_back({ body, tag });
}

void PhysicsSystem::removeRigidBody(std::shared_ptr<RigidBody> body) {
    bodies.erase(std::remove_if(bodies.begin(), bodies.end(),
        [body](const RigidBodyEntry& e) { return e.body == body; }),
        bodies.end());
}

void PhysicsSystem::update(float deltaTime) {
    // Предварительная интеграция
    for (auto& entry : bodies) {
        entry.body->isGrounded = false;
    }

    // Предварительная интеграция
    for (auto& entry : bodies) {
        entry.body->integrate(deltaTime, gravity);
    }


    for (int iter = 0; iter < iterations; ++iter) {
        // Синхронизируем коллайдеры с актуальными позициями тел
        for (auto& entry : bodies) {
            if (entry.body->collider) {
                entry.body->collider->getTransform() = *entry.body->transform;
            }
        }

        // Обнаружение и разрешение коллизий
        for (size_t i = 0; i < bodies.size(); ++i) {
            for (size_t j = i + 1; j < bodies.size(); ++j) {
                auto& bodyA = bodies[i].body;
                auto& bodyB = bodies[j].body;
                if (bodyA->invMass == 0.0f && bodyB->invMass == 0.0f) continue;
                if (!bodyA->collider || !bodyB->collider) continue;

                CollisionInfo info;
                if (CollisionSystem::getInstance().checkCollision(
                    bodyA->collider, bodyB->collider, &info)) {
                    resolveCollision(bodyA, bodyB, info);
                }
            }
        }
    }

    // Финальная синхронизация (на всякий случай)
    for (auto& entry : bodies) {
        if (entry.body->collider) {
            entry.body->collider->getTransform() = *entry.body->transform;
        }
    }
}
void PhysicsSystem::resolveCollision(std::shared_ptr<RigidBody> bodyA,
    std::shared_ptr<RigidBody> bodyB,
    const CollisionInfo& info) {
    float totalInvMass = bodyA->invMass + bodyB->invMass;
    if (totalInvMass == 0.0f) return;

    // Slop и коэффициент коррекции уменьшаем
    const float slop = 0.02f;          // чуть больше допустимого проникновения
    float penetration = std::max(info.depth - slop, 0.0f);
    if (penetration > 0.0f) {
        float factor = 0.2f;
        if (penetration > 0.1f) factor = 0.4f; // усиленная коррекция при глубоком проникновении
        glm::vec3 correction = info.normal * (penetration / totalInvMass) * factor;
        if (bodyA->invMass > 0.0f)
            bodyA->transform->position += correction * bodyA->invMass;
        if (bodyB->invMass > 0.0f)
            bodyB->transform->position -= correction * bodyB->invMass;
    }



    glm::vec3 relativeVelocity = bodyA->velocity - bodyB->velocity;
    float velAlongNormal = glm::dot(relativeVelocity, info.normal);

    if (velAlongNormal > 0.0f) return;

    float e = std::min(bodyA->restitution, bodyB->restitution);
    float j = -(1.0f + e) * velAlongNormal / totalInvMass;
    glm::vec3 impulse = j * info.normal;

    if (bodyA->invMass > 0.0f)
        bodyA->velocity += impulse * bodyA->invMass;
    if (bodyB->invMass > 0.0f)
        bodyB->velocity -= impulse * bodyB->invMass;

    // Гашение горизонтальной скорости при контакте с поверхностью
    // Условие смягчаем: считаем любой контакт с нормалью, направленной вверх, как пол
    if (info.normal.y > 0.7f && bodyA->invMass > 0.0f) {
        bodyA->velocity.x *= 0.9f;
        bodyA->velocity.z *= 0.9f;
        bodyA->isGrounded = true;
    }
    if (info.normal.y < -0.7f && bodyB->invMass > 0.0f) {
        bodyB->velocity.x *= 0.9f;
        bodyB->velocity.z *= 0.9f;
        bodyB->isGrounded = true;
    }
}