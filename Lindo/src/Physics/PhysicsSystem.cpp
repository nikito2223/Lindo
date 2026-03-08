// PhysicsSystem.cpp
#include "PhysicsSystem.h"
#include <algorithm>
#include <Physics/Collider/CollisionSystem.h>

PhysicsSystem& PhysicsSystem::getInstance() {
    static PhysicsSystem instance;
    return instance;
}

void PhysicsSystem::addRigidBody(RigidBody* body, const std::string& tag) {
    bodies.push_back({ body, tag });
}

void PhysicsSystem::removeRigidBody(RigidBody* body) {
    bodies.erase(std::remove_if(bodies.begin(), bodies.end(),
        [body](const RigidBodyEntry& e) { return e.body == body; }),
        bodies.end());
}

void PhysicsSystem::update(float deltaTime) {
    // Предварительная интеграция
    for (auto& entry : bodies) {
        entry.body->isGrounded = false;
        entry.body->integrate(deltaTime, gravity);
    }


    for (int iter = 0; iter < iterations; ++iter) {
        // Синхронизируем коллайдеры
        for (auto& entry : bodies) {
            if (entry.body->collider) {
                entry.body->collider->setPosition(entry.body->owner->transform.position);
            }
        }

        // Обнаружение коллизий
        for (size_t i = 0; i < bodies.size(); ++i) {
            for (size_t j = i + 1; j < bodies.size(); ++j) {
                auto& bodyA = bodies[i].body;
                auto& bodyB = bodies[j].body;

                if (!bodyA->collider || !bodyB->collider) continue;

                CollisionInfo info;
                if (CollisionSystem::getInstance().checkCollision(bodyA->collider, bodyB->collider, &info)) {

                    // ЕСЛИ ХОТЯ БЫ ОДИН ИЗ НИХ ТРИГГЕР
                    if (bodyA->collider->getTrigger() || bodyB->collider->getTrigger()) {

                        // Вызываем действия у обоих (если они назначены)
                        bodyA->collider->fireTriggerEvent(bodyB->collider);
                        bodyB->collider->fireTriggerEvent(bodyA->collider);

                        // ВАЖНО: resolveCollision НЕ ВЫЗЫВАЕМ, чтобы не было физического отскока
                    }
                    else {
                        // ОБЫЧНАЯ ФИЗИКА (отскоки)
                        resolveCollision(bodyA, bodyB, info);
                    }
                }
            }
        }
    }

    // Финальная синхронизация (на всякий случай)
    for (auto& entry : bodies) {
        if (entry.body->collider) {
            entry.body->collider->setPosition(entry.body->owner->transform.position);
        }
    }
}
void PhysicsSystem::resolveCollision(RigidBody* bodyA, RigidBody* bodyB, const CollisionInfo& info) {
    
    // --- ЛОГИКА ТРИГГЕРОВ ---
    // Если хотя бы один из коллайдеров — триггер, мы НЕ вычисляем физику (отскоки и депенатрацию)
    if (bodyA->collider->getTrigger() || bodyB->collider->getTrigger()) {
        // Здесь можно вызвать callback-систему:
        // OnTriggerOverlap(bodyA, bodyB); 
        return;
    }
    // ------------------------

    float totalInvMass = bodyA->invMass + bodyB->invMass;
    if (totalInvMass == 0.0f) return;

    // 1. Позиционная коррекция (выталкивание объектов друг из друга)
    const float slop = 0.02f;
    float penetration = std::max(info.depth - slop, 0.0f);
    if (penetration > 0.0f) {
        float factor = (penetration > 0.1f) ? 0.4f : 0.2f;
        glm::vec3 correction = info.normal * (penetration / totalInvMass) * factor;

        if (bodyA->invMass > 0.0f)
            bodyA->owner->transform.position += correction * bodyA->invMass;
        if (bodyB->invMass > 0.0f)
            bodyB->owner->transform.position -= correction * bodyB->invMass;
    }

    // 2. Расчет импульса (отскок)
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

    // 3. Трение и приземление (Grounding)
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

bool PhysicsSystem::raycast(const glm::vec3& origin,
    const glm::vec3& direction,
    float maxDistance,
    RaycastHit& hit,
    const std::string& tagFilter,
    RigidBody* ignoreBody,
    bool ignoreTriggers) const { // Реализация

    float closest = maxDistance;
    bool found = false;
    glm::vec3 normDir = glm::normalize(direction);

    for (const auto& entry : bodies) {
        const auto& body = entry.body;
        if (!body->collider) continue;
        if (ignoreBody && body == ignoreBody) continue;
        if (!tagFilter.empty() && entry.tag != tagFilter) continue;

        // ВОТ ОНО: Пропускаем, если это триггер и мы просили их игнорировать
        if (ignoreTriggers && body->collider->getTrigger()) continue;

        RaycastHit tempHit;
        if (body->collider->intersectRay(origin, normDir, maxDistance, tempHit)) {
            if (tempHit.distance < closest) {
                closest = tempHit.distance;
                hit = tempHit;
                hit.body = body;
                hit.tag = entry.tag;
                found = true;
            }
        }
    }
    return found;
}

std::vector<RaycastHit> PhysicsSystem::raycastAll(const glm::vec3& origin,
    const glm::vec3& direction,
    float maxDistance,
    const std::string& tagFilter,
    RigidBody* ignoreBody) const {
    std::vector<RaycastHit> hits;
    glm::vec3 normDir = glm::normalize(direction);

    for (const auto& entry : bodies) {
        const auto& body = entry.body;
        if (!body->collider) continue;
        if (ignoreBody && body == ignoreBody) continue;
        if (!tagFilter.empty() && entry.tag != tagFilter) continue;

        RaycastHit hit;
        if (body->collider->intersectRay(origin, normDir, maxDistance, hit)) {
            hit.body = body;
            hit.tag = entry.tag;
            hits.push_back(hit);
        }
    }

    // Сортируем по расстоянию
    std::sort(hits.begin(), hits.end(),
        [](const RaycastHit& a, const RaycastHit& b) {
            return a.distance < b.distance;
        });
    return hits;
}
