#include "PhysicsSystem.h"
#include <Physics/Collider/Collider.h>
#include <Physics/Collider/BoxCollider.h>
#include <Physics/Collider/SphereCollider.h>
#include <Physics/Collider/CapsuleCollider.h>
#include <Physics/GravityField.h>
#include <Component/Physhcs/RigidBody.h>
#include "Core/Time/Time.h"
#include <Component/GameObject/GameObject.h>
#include <algorithm>
#include <cmath>

namespace Lindo {
    namespace Components {
        namespace Physics {

            PhysicsSystem& PhysicsSystem::GetInstance() {
                static PhysicsSystem instance;
                return instance;
            }

            void PhysicsSystem::RegisterCollider(Collider* collider) {
                if (!collider) return;
                if (std::find(colliders.begin(), colliders.end(), collider) == colliders.end()) {
                    colliders.push_back(collider);
                }
            }

            void PhysicsSystem::UnregisterCollider(Collider* collider) {
                auto it = std::find(colliders.begin(), colliders.end(), collider);
                if (it != colliders.end()) {
                    colliders.erase(it);
                }
            }

            void PhysicsSystem::RegisterRigidBody(RigidBody* body) {
                if (!body) return;
                if (std::find(rigidBodies.begin(), rigidBodies.end(), body) == rigidBodies.end()) {
                    rigidBodies.push_back(body);
                }
            }

            void PhysicsSystem::UnregisterRigidBody(RigidBody* body) {
                auto it = std::find(rigidBodies.begin(), rigidBodies.end(), body);
                if (it != rigidBodies.end()) {
                    rigidBodies.erase(it);
                }
            }

            void PhysicsSystem::RegisterGravityField(GravityField* field) {
                if (!field) return;
                if (std::find(gravityFields.begin(), gravityFields.end(), field) == gravityFields.end()) {
                    gravityFields.push_back(field);
                }
            }

            void PhysicsSystem::UnregisterGravityField(GravityField* field) {
                auto it = std::find(gravityFields.begin(), gravityFields.end(), field);
                if (it != gravityFields.end()) {
                    gravityFields.erase(it);
                }
            }

            void PhysicsSystem::Clear() {
                colliders.clear();
                rigidBodies.clear();
                gravityFields.clear();
            }

            void PhysicsSystem::SetGlobalGravity(const glm::vec3& gravity) {
                globalGravity = gravity;
                GravityField::s_globalGravity = gravity;
            }

            glm::vec3 PhysicsSystem::GetGlobalGravity() const {
                return globalGravity;
            }

            void PhysicsSystem::SetIterations(int iterations) {
                solverIterations = std::max(1, iterations);
            }

            int PhysicsSystem::GetIterations() const {
                return solverIterations;
            }

            void PhysicsSystem::SetVelocityIterations(int iterations) {
                solverIterations = std::max(1, iterations);
            }

            void PhysicsSystem::SetPositionIterations(int iterations) {
                solverIterations = std::max(1, iterations);
            }

            std::vector<PhysicsSystem::BroadPair> PhysicsSystem::BroadPhase() const {
                std::vector<BroadPair> pairs;
                for (size_t i = 0; i < colliders.size(); ++i) {
                    Collider* a = colliders[i];
                    if (!a || !a->IsEnabled()) continue;
                    Lindo::Math::AABB aabbA = a->GetAABB();

                    for (size_t j = i + 1; j < colliders.size(); ++j) {
                        Collider* b = colliders[j];
                        if (!b || !b->IsEnabled()) continue;
                        if (a->gameObject && (a->gameObject == b->gameObject)) continue;

                        Lindo::Math::AABB aabbB = b->GetAABB();
                        if (aabbA.intersectAABB(aabbB)) {
                            pairs.push_back({ a, b });
                        }
                    }
                }
                return pairs;
            }

            bool PhysicsSystem::NarrowPhase(Collider* a, Collider* b, Contact& outContact) const {
                CollisionInfo info;
                if (!a->CheckCollision(b, info)) {
                    return false;
                }

                outContact.colliderA = a;
                outContact.colliderB = b;
                outContact.point = info.contactPoint;
                outContact.normal = info.contactNormal;
                outContact.penetration = info.penetrationDepth;

                float friction, restitution;
                ComputeContactProperties(outContact, friction, restitution);
                outContact.friction = friction;
                outContact.restitution = restitution;
                return true;
            }

            void PhysicsSystem::ComputeContactProperties(const Contact& contact,
                float& outFriction, float& outRestitution) const {
                float frA = contact.colliderA ? contact.colliderA->GetFriction() : 0.5f;
                float frB = contact.colliderB ? contact.colliderB->GetFriction() : 0.5f;
                float reA = contact.colliderA ? contact.colliderA->GetRestitution() : 0.2f;
                float reB = contact.colliderB ? contact.colliderB->GetRestitution() : 0.2f;

                outFriction = frA * frB;
                outRestitution = std::max(reA, reB);
            }

            void PhysicsSystem::ResolveContacts(std::vector<Contact>& contacts) {
                if (contacts.empty()) return;

                // 1. Установка флагов заземления
                for (Contact& contact : contacts) {
                    if (!contact.colliderA || !contact.colliderB) continue;
                    if (contact.colliderA->IsTrigger() || contact.colliderB->IsTrigger()) continue;

                    RigidBody* bodyA = contact.colliderA->gameObject ? contact.colliderA->gameObject->getComponent<RigidBody>() : nullptr;
                    RigidBody* bodyB = contact.colliderB->gameObject ? contact.colliderB->gameObject->getComponent<RigidBody>() : nullptr;

                    glm::vec3 normal = contact.normal;
                    if (glm::length(normal) < 1e-6f) continue;
                    normal = glm::normalize(normal);

                    if (bodyA && normal.y < -0.5f) bodyA->isGrounded = true;
                    if (bodyB && normal.y > 0.5f) bodyB->isGrounded = true;
                }

                // 2. Решение скоростей (Velocity Solver)
                for (int iter = 0; iter < solverIterations; ++iter) {
                    for (Contact& contact : contacts) {
                        if (!contact.colliderA || !contact.colliderB) continue;
                        if (contact.colliderA->IsTrigger() || contact.colliderB->IsTrigger()) continue;

                        RigidBody* bodyA = contact.colliderA->gameObject ? contact.colliderA->gameObject->getComponent<RigidBody>() : nullptr;
                        RigidBody* bodyB = contact.colliderB->gameObject ? contact.colliderB->gameObject->getComponent<RigidBody>() : nullptr;

                        float invMassA = (bodyA && !bodyA->IsKinematic()) ? bodyA->GetInvMass() : 0.0f;
                        float invMassB = (bodyB && !bodyB->IsKinematic()) ? bodyB->GetInvMass() : 0.0f;
                        float invMassSum = invMassA + invMassB;

                        if (invMassSum <= 0.0f) continue;

                        glm::vec3 normal = contact.normal;
                        if (glm::length(normal) < 1e-6f) continue;
                        normal = glm::normalize(normal);

                        glm::vec3 velA = bodyA ? bodyA->GetVelocity() : glm::vec3(0.0f);
                        glm::vec3 velB = bodyB ? bodyB->GetVelocity() : glm::vec3(0.0f);
                        glm::vec3 relVel = velB - velA;

                        float velAlongNormal = glm::dot(relVel, normal);

                        if (velAlongNormal < 0.0f) {
                            float e = (std::abs(velAlongNormal) < 0.2f) ? 0.0f : contact.restitution;
                            float j = -(1.0f + e) * velAlongNormal / invMassSum;
                            glm::vec3 impulse = j * normal;

                            if (bodyA && !bodyA->IsKinematic()) bodyA->applyImpulse(-impulse);
                            if (bodyB && !bodyB->IsKinematic()) bodyB->applyImpulse(impulse);

                            // Трение
                            relVel = (bodyB ? bodyB->GetVelocity() : glm::vec3(0.0f)) -
                                (bodyA ? bodyA->GetVelocity() : glm::vec3(0.0f));
                            glm::vec3 tangent = relVel - glm::dot(relVel, normal) * normal;
                            if (glm::length(tangent) > 1e-6f) {
                                tangent = glm::normalize(tangent);
                                float jt = -glm::dot(relVel, tangent) / invMassSum;
                                float maxFriction = contact.friction * std::fabs(j);
                                glm::vec3 frictionImpulse = tangent * std::clamp(jt, -maxFriction, maxFriction);

                                if (bodyA && !bodyA->IsKinematic()) bodyA->applyImpulse(-frictionImpulse);
                                if (bodyB && !bodyB->IsKinematic()) bodyB->applyImpulse(frictionImpulse);
                            }
                        }
                    }
                }

                // 3. Коррекция проникновения (Position Projection)
                const float slop = 0.01f;   // Допустимый зазор для предотвращения тряски
                const float percent = 0.8f; // Процент выталкивания за один кадр

                for (Contact& contact : contacts) {
                    if (!contact.colliderA || !contact.colliderB) continue;
                    if (contact.colliderA->IsTrigger() || contact.colliderB->IsTrigger()) continue;

                    RigidBody* bodyA = contact.colliderA->gameObject ? contact.colliderA->gameObject->getComponent<RigidBody>() : nullptr;
                    RigidBody* bodyB = contact.colliderB->gameObject ? contact.colliderB->gameObject->getComponent<RigidBody>() : nullptr;

                    float invMassA = (bodyA && !bodyA->IsKinematic()) ? bodyA->GetInvMass() : 0.0f;
                    float invMassB = (bodyB && !bodyB->IsKinematic()) ? bodyB->GetInvMass() : 0.0f;
                    float invMassSum = invMassA + invMassB;

                    if (invMassSum <= 0.0f) continue;

                    glm::vec3 normal = contact.normal;
                    if (glm::length(normal) < 1e-6f) continue;
                    normal = glm::normalize(normal);

                    // Clamp how much penetration we correct for in a single
                    // step. Without this, a body that ends up deeply
                    // overlapping another (e.g. after a hitch, or briefly
                    // tunneling through on a fast/short-lived contact) gets
                    // shoved all the way back out to the surface instantly,
                    // which looks like a teleport. maxPenetration was already
                    // exposed via SetMaxPenetration() but never actually used
                    // here.
                    float penetration = std::min(contact.penetration, maxPenetration);
                    if (penetration > slop) {
                        glm::vec3 correction = normal * ((penetration - slop) / invMassSum) * percent;

                        if (bodyA && !bodyA->IsKinematic() && bodyA->gameObject) {
                            bodyA->gameObject->transform.position -= correction * invMassA;
                        }
                        if (bodyB && !bodyB->IsKinematic() && bodyB->gameObject) {
                            bodyB->gameObject->transform.position += correction * invMassB;
                        }
                    }
                }
            }

            glm::vec3 PhysicsSystem::ComputeGravityAt(const glm::vec3& worldPos) const {
                glm::vec3 gravity = globalGravity;
                for (auto* field : gravityFields) {
                    if (!field || !field->IsEnabled()) continue;
                    gravity += field->GetAccelerationAt(worldPos);
                }
                return gravity;
            }

            void PhysicsSystem::UpdateCollisionEvents(Collider* a, Collider* b, const Contact& contact) {
                if (!a || !b) return;

                if (a->IsTrigger() || b->IsTrigger()) {
                    const auto& aTriggers = a->GetCurrentTriggers();
                    const auto& bTriggers = b->GetCurrentTriggers();
                    bool aHasB = std::find(aTriggers.begin(), aTriggers.end(), b) != aTriggers.end();
                    bool bHasA = std::find(bTriggers.begin(), bTriggers.end(), a) != bTriggers.end();

                    if (!aHasB) {
                        a->OnTriggerEnter(b);
                        b->OnTriggerEnter(a);
                    }
                    else {
                        a->OnTriggerStay(b);
                        b->OnTriggerStay(a);
                    }
                    return;
                }

                const auto& aCollisions = a->GetCurrentCollisions();
                const auto& bCollisions = b->GetCurrentCollisions();
                bool aHasB = std::find(aCollisions.begin(), aCollisions.end(), b) != aCollisions.end();
                bool bHasA = std::find(bCollisions.begin(), bCollisions.end(), a) != bCollisions.end();

                CollisionInfo info;
                info.other = b;
                info.contactPoint = contact.point;
                info.contactNormal = contact.normal;
                info.penetrationDepth = contact.penetration;
                info.relativeVelocity = 0.0f;

                if (!aHasB) {
                    a->OnCollisionEnter(b, info);
                }
                else {
                    a->OnCollisionStay(b, info);
                }
                if (!bHasA) {
                    b->OnCollisionEnter(a, info);
                }
                else {
                    b->OnCollisionStay(a, info);
                }
            }

            void PhysicsSystem::Step() {
                float dt = Lindo::Time::GetFixedDeltaTime();
                if (dt <= 0.0f) return;

                // Сброс флага земли
                for (auto* body : rigidBodies) {
                    if (body) body->isGrounded = false;
                }

                // 1. Интеграция физических тел
                for (auto* body : rigidBodies) {
                    if (!body || body->isSleeping || body->IsKinematic()) continue;
                    body->integrate(ComputeGravityAt(body->gameObject ? body->gameObject->transform.position : glm::vec3(0.0f)));
                }

                // 2. Broad phase
                auto pairs = BroadPhase();

                // 3. Narrow phase
                std::vector<Contact> contacts;
                for (const auto& pair : pairs) {
                    Contact contact;
                    if (NarrowPhase(pair.a, pair.b, contact)) {
                        contacts.push_back(contact);
                        UpdateCollisionEvents(pair.a, pair.b, contact);
                    }
                }

                // 4. Солвер импульсов и выталкивания
                if (useSolver) {
                    ResolveContacts(contacts);
                }
            }

            bool PhysicsSystem::Raycast(const glm::vec3& origin, const glm::vec3& direction,
                float maxDistance, Collider*& outHit, glm::vec3& outPoint,
                glm::vec3& outNormal, float& outDistance) const {
                glm::vec3 dir = glm::normalize(direction);
                bool hit = false;
                float closest = maxDistance;

                for (Collider* collider : colliders) {
                    if (!collider || !collider->IsEnabled()) continue;
                    Lindo::Math::AABB aabb = collider->GetAABB();
                    float tmin, tmax;
                    if (!aabb.intersectRay(origin, dir, tmin, tmax)) continue;
                    if (tmin < 0.0f) tmin = 0.0f;
                    if (tmin > closest) continue;

                    closest = tmin;
                    outHit = collider;
                    outDistance = tmin;
                    outPoint = origin + dir * tmin;
                    outNormal = outPoint - collider->GetWorldCenter();
                    if (glm::length(outNormal) > 1e-6f) {
                        outNormal = glm::normalize(outNormal);
                    }
                    else {
                        outNormal = -dir;
                    }
                    hit = true;
                }
                return hit;
            }

            bool PhysicsSystem::OverlapPoint(const glm::vec3& point, Collider*& outHit) const {
                for (Collider* collider : colliders) {
                    if (!collider || !collider->IsEnabled()) continue;
                    Lindo::Math::AABB aabb = collider->GetAABB();
                    if (aabb.distanceToPoint(point) <= 0.0f) {
                        outHit = collider;
                        return true;
                    }
                }
                return false;
            }

            std::vector<Collider*> PhysicsSystem::OverlapSphere(const glm::vec3& center, float radius) const {
                std::vector<Collider*> result;
                for (Collider* collider : colliders) {
                    if (!collider || !collider->IsEnabled()) continue;
                    Lindo::Math::AABB aabb = collider->GetAABB();
                    if (aabb.intersectSphere(center, radius)) {
                        result.push_back(collider);
                    }
                }
                return result;
            }

            std::vector<Collider*> PhysicsSystem::OverlapBox(const glm::vec3& center, const glm::vec3& halfExtents) const {
                glm::vec3 min = center - halfExtents;
                glm::vec3 max = center + halfExtents;
                Lindo::Math::AABB query(min, max);

                std::vector<Collider*> result;
                for (Collider* collider : colliders) {
                    if (!collider || !collider->IsEnabled()) continue;
                    Lindo::Math::AABB aabb = collider->GetAABB();
                    if (aabb.intersectAABB(query)) {
                        result.push_back(collider);
                    }
                }
                return result;
            }

        }
    }
}