#include "RigidBody.h"
#include <Component/GameObject/GameObject.h>
#include <Component/Physhcs/Colliders/Collider.h>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <Physics/PhysicsSystem.h>
#include <Core/Time/Time.h>

namespace Lindo {
    namespace Components {
        namespace Physics {

            RigidBody::RigidBody(float mass)
                : mass(std::max(mass, 0.0001f)),
                invMass(1.0f / std::max(mass, 0.0001f)),
                velocity(0.0f),
                angularVelocity(0.0f),
                acceleration(0.0f),
                forceAccumulator(0.0f),
                torqueAccumulator(0.0f) {
            }

            void RigidBody::SetMass(float newMass) {
                mass = std::max(newMass, 0.0001f);
                invMass = (mass > 0.0f) ? (1.0f / mass) : 0.0f;
            }

            void RigidBody::OnStart() {
                PhysicsSystem::GetInstance().RegisterRigidBody(this);
            }

            void RigidBody::OnDestroy() {
                PhysicsSystem::GetInstance().UnregisterRigidBody(this);
            }

            void RigidBody::applyForce(const glm::vec3& force) {
                if (isKinematic) return;
                forceAccumulator += force;
                Wake();
            }

            void RigidBody::applyForceAtPoint(const glm::vec3& force, const glm::vec3& worldPoint) {
                if (isKinematic) return;
                forceAccumulator += force;
                if (gameObject) {
                    glm::vec3 r = worldPoint - gameObject->transform.position;
                    torqueAccumulator += glm::cross(r, force);
                }
                Wake();
            }

            void RigidBody::applyImpulse(const glm::vec3& impulse) {
                if (isKinematic) return;
                velocity += impulse * invMass;
                Wake();
            }

            void RigidBody::applyImpulseAtPoint(const glm::vec3& impulse, const glm::vec3& worldPoint) {
                if (isKinematic) return;
                velocity += impulse * invMass;
                if (gameObject) {
                    glm::vec3 r = worldPoint - gameObject->transform.position;
                    glm::mat3 invInertia = glm::inverse(GetInertiaTensor());
                    angularVelocity += invInertia * glm::cross(r, impulse);
                }
                Wake();
            }

            void RigidBody::applyTorque(const glm::vec3& torque) {
                if (isKinematic) return;
                torqueAccumulator += torque;
                Wake();
            }

            void RigidBody::clearForces() {
                forceAccumulator = glm::vec3(0.0f);
                torqueAccumulator = glm::vec3(0.0f);
            }

            void RigidBody::OnUpdate(const glm::vec3& gravity) {}

            void RigidBody::OnUpdate() {}

            glm::mat3 RigidBody::GetInertiaTensor() const {
                glm::vec3 half(0.5f);
                if (gameObject) {
                    half = glm::vec3(0.5f) * gameObject->transform.scale;
                }
                float m = mass;
                float ix = (1.0f / 12.0f) * m * (half.y * half.y + half.z * half.z) * 4.0f;
                float iy = (1.0f / 12.0f) * m * (half.x * half.x + half.z * half.z) * 4.0f;
                float iz = (1.0f / 12.0f) * m * (half.x * half.x + half.y * half.y) * 4.0f;
                return glm::mat3(ix, 0, 0,
                    0, iy, 0,
                    0, 0, iz);
            }

            void RigidBody::integrate(const glm::vec3& gravity) {
                if (!gameObject || invMass == 0.0f || isKinematic || isSleeping) return;

                float dt = Lindo::Time::GetFixedDeltaTime();

                // 1. Применяем гравитацию
                if (useGravity) {
                    forceAccumulator += gravity * gravityScale * mass;
                }

                // 2. Линейное и угловое ускорение
                if (forceAccumulator != glm::vec3(0.0f)) {
                    acceleration = forceAccumulator * invMass;
                    velocity += acceleration * dt;
                }

                if (torqueAccumulator != glm::vec3(0.0f)) {
                    glm::mat3 invInertia = glm::inverse(GetInertiaTensor());
                    angularVelocity += invInertia * torqueAccumulator * dt;
                }

                ApplyDamping();
                ClampVelocity(25.0f);

                // 3. Шаг по позиции с поддержкой CCD (Continuous Collision Detection / Raycast Sweep)

                glm::vec3 moveVector = velocity * dt;
                float moveDist = glm::length(moveVector);

                if (moveDist > 1e-4f) {
                    Collider* hitCollider = nullptr;
                    glm::vec3 hitPoint(0.0f);
                    glm::vec3 hitNormal(0.0f);
                    float hitDist = 0.0f;
                
                    // Передаем `this` последним аргументом, чтобы не проверять столкновение с самим собой
                    if (PhysicsSystem::GetInstance().Raycast(gameObject->transform.position, 
                                                             glm::normalize(moveVector), 
                                                             moveDist, hitCollider, hitPoint, hitNormal, hitDist, this)) {
                                                            
                        // Проверяем, что найденный коллайдер не принадлежит этому же GameObject
                        if (hitCollider && hitCollider->gameObject != gameObject) {
                            gameObject->transform.position += glm::normalize(moveVector) * std::max(0.0f, hitDist - 0.01f);

                            float velAlongNormal = glm::dot(velocity, hitNormal);
                            if (velAlongNormal < 0.0f) {
                                velocity -= hitNormal * velAlongNormal;
                            }
                        } else {
                            gameObject->transform.position += moveVector;
                        }
                    } else {
                        gameObject->transform.position += moveVector;
                    }
                }

                // 4. Вращение
                if (glm::length(angularVelocity) > 1e-6f) {
                    glm::vec3 eulerDeg = gameObject->transform.rotation;
                    glm::vec3 deltaDeg = glm::degrees(angularVelocity) * dt;
                    gameObject->transform.rotation = eulerDeg + deltaDeg;
                }

                ClearAccumulators();
                UpdateSleepState();
            }

            void RigidBody::ClearAccumulators() {
                forceAccumulator = glm::vec3(0.0f);
                torqueAccumulator = glm::vec3(0.0f);
            }

            void RigidBody::ApplyDamping() {
                float dt = Lindo::Time::GetFixedDeltaTime();
                float linearF = std::max(0.0f, 1.0f - linearDamping * dt * 60.0f);
                float angularF = std::max(0.0f, 1.0f - angularDamping * dt * 60.0f);
                velocity *= linearF;
                angularVelocity *= angularF;
            }

            void RigidBody::ClampVelocity(float maxSpeed) {
                float speed = glm::length(velocity);
                if (speed > maxSpeed) {
                    velocity = glm::normalize(velocity) * maxSpeed;
                }
                float angularSpeed = glm::length(angularVelocity);
                if (angularSpeed > maxSpeed * 2.0f) {
                    angularVelocity = glm::normalize(angularVelocity) * (maxSpeed * 2.0f);
                }
            }

            void RigidBody::UpdateSleepState() {
                float dt = Lindo::Time::GetFixedDeltaTime();
                float speed = glm::length(velocity);
                float angularSpeed = glm::length(angularVelocity);
                if (speed < sleepThreshold && angularSpeed < sleepThreshold) {
                    sleepTimer += dt;
                    if (sleepTimer > 0.5f) {
                        isSleeping = true;
                        velocity = glm::vec3(0.0f);
                        angularVelocity = glm::vec3(0.0f);
                    }
                }
                else {
                    sleepTimer = 0.0f;
                    isSleeping = false;
                }
            }

        }
    }
}