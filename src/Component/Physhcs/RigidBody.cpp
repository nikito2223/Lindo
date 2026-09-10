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
                // PhysicsSystem::Step() advances the world using the FIXED
                // timestep, so integration must use the same value. Using the
                // variable render-frame delta here means that after a frame
                // hitch (or on a Step() that fires more than once per frame
                // to catch up) the body gets integrated with a much larger dt
                // than the collision solver expects, so it can plunge deep
                // into/through another collider in a single step before the
                // solver ever sees it. The next Step() then finds a huge
                // penetration and shoves the body all the way back out in one
                // go, which reads as "falls to the center, then teleports
                // back up instantly".
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

                // 3. Шаг по позиции
                gameObject->transform.position += velocity * dt;

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