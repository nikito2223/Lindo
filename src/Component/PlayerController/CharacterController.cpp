#include "CharacterController.h"
#include "Component/GameObject/GameObject.h"
#include <Physics/PhysicsSystem.h>
#include <algorithm>
#include <cmath>
#include <Component/Physhcs/RigidBody.h>
#include <Core/Time/Time.h>

namespace Lindo {
    namespace Components {
        namespace Character {

            CharacterController::CharacterController() {
                collider.SetHeight(1.8f);
                collider.SetRadius(0.4f);
                originalHeight = collider.GetHeight();
                originalCrouchHeight = originalHeight * 0.6f;
            }

            CharacterController::CharacterController(const CharacterMovementSettings& settings)
                : settings(settings) {
                collider.SetHeight(1.8f);
                collider.SetRadius(0.4f);
                originalHeight = collider.GetHeight();
                originalCrouchHeight = originalHeight * 0.6f;
            }

            void CharacterController::OnStart() {
                lastSafePosition = getPosition();

                // Set up the internal capsule collider as a physical collider.
                collider.gameObject = gameObject;
                collider.SetOffset(glm::vec3(0.0f, collider.GetHeight() * 0.5f, 0.0f));
                collider.OnStart();
            }

            void CharacterController::OnUpdate() {
                float dt = Lindo::Time::GetDeltaTime();

                if (dt <= 0.0f) return;

                if (gameObject->transform.position.y <= -100.0f) {
                    // 1. Возвращаем позицию
                    gameObject->transform.position = { 0.0f, 10.0f, 0.0f };
                }

                UpdateGroundStatus();
                ApplyMovement(currentInput);
                UpdatePhysics();
                UpdateCurrentSpeed();

                glm::vec3 oldPosition = getPosition();
                glm::vec3 newPosition = oldPosition + state.velocity * dt;
                setPosition(newPosition);

                ResolveCollisions();

                // TODO: Check collision with world
                lastSafePosition = getPosition();

                HandleAutoOrientation();

                if (!state.wasGrounded && state.isGrounded && onLand) {
                    onLand();
                }
                state.wasGrounded = state.isGrounded;
            }

            void CharacterController::OnDestroy() {
                // Очистка, физическая подсистема все обработает сама
            }

            void CharacterController::Move(glm::vec3 inputDirection, bool isRunning) {
                currentInput = inputDirection;
                isRunningInput = isRunning;

                // Изменение стейта движения на основе ввода
                if (state.isGrounded) {
                    if (isRunningInput && movementMode != MovementMode::Running && movementMode != MovementMode::Crouching) {
                        SetMovementMode(MovementMode::Running);
                    }
                    else if (!isRunningInput && movementMode == MovementMode::Running) {
                        SetMovementMode(MovementMode::Walking);
                    }
                }
            }

            void CharacterController::Rotate(glm::vec3 lookDirection) {
                if (orientationMode == AutoOrientationMode::CameraDirection) {
                    // Поворачиваем объект к вектору взгляда
                    if (glm::length(lookDirection) > 0.1f) {
                        // Здесь установка поворота в gameObject->transform
                    }
                }
            }

            bool CharacterController::Jump() {
                if (!state.isGrounded || state.isJumping) return false;

                if (movementMode == MovementMode::Crouching) {
                    UnCrouch();
                }

                float jumpVelocity = sqrtf(2.0f * 9.81f * settings.jumpHeight);
                state.velocity.y = jumpVelocity;
                state.isJumping = true;
                state.isGrounded = false;

                if (onJump) onJump();
                return true;
            }

            bool CharacterController::Crouch() {
                if (movementMode == MovementMode::Crouching) return false;

                collider.SetHeight(originalCrouchHeight);
                collider.SetOffset(glm::vec3(0.0f, collider.GetHeight() * 0.5f, 0.0f));
                SetMovementMode(MovementMode::Crouching);

                if (onCrouch) onCrouch(true);
                return true;
            }

            bool CharacterController::UnCrouch() {
                if (movementMode != MovementMode::Crouching) return false;

                collider.SetHeight(originalHeight);
                collider.SetOffset(glm::vec3(0.0f, collider.GetHeight() * 0.5f, 0.0f));
                SetMovementMode(MovementMode::Walking);

                if (onCrouch) onCrouch(false);
                return true;
            }

            bool CharacterController::Slide() {
                if (!state.isGrounded || movementMode == MovementMode::Sliding) return false;

                SetMovementMode(MovementMode::Sliding);
                glm::vec3 slideDir = glm::normalize(glm::vec3(state.velocity.x, 0.0f, state.velocity.z));
                AddImpulse(slideDir * settings.slideSpeed);
                return true;
            }

            void CharacterController::StopMovement() {
                state.velocity = glm::vec3(0.0f);
                currentInput = glm::vec3(0.0f);
            }

            void CharacterController::Teleport(const glm::vec3& position) {
                setPosition(position);
                collider.setPosition(position);
                lastSafePosition = position;
                StopMovement();
            }

            void CharacterController::SetMovementMode(MovementMode mode) {
                if (movementMode == mode) return;
                movementMode = mode;

                // Специфические настройки для режимов
                switch (mode) {
                case MovementMode::Flying:
                    state.velocity.y = 0;
                    break;
                case MovementMode::Swimming:
                    state.velocity.y *= 0.5f;
                    break;
                default:
                    break;
                }
            }

            void CharacterController::SetAutoOrientationMode(AutoOrientationMode mode) {
                orientationMode = mode;
            }

            void CharacterController::AddImpulse(const glm::vec3& impulse) {
                state.velocity += impulse;
            }

            void CharacterController::SetVelocity(const glm::vec3& newVelocity) {
                state.velocity = newVelocity;
            }

            void CharacterController::SetMovementSettings(const CharacterMovementSettings& newSettings) {
                settings = newSettings;
            }

            // Private methods
            void CharacterController::UpdateGroundStatus() {
                state.wasGrounded = state.isGrounded;
                // The actual grounded state is determined during collision resolution.
            }

            void CharacterController::UpdatePhysics() {
                if (!state.isGrounded) {
                    ApplyGravity();
                }
            }

            void CharacterController::ApplyGravity() {
                if (movementMode == MovementMode::Flying || movementMode == MovementMode::Swimming) {
                    return;
                }

                float dt = Lindo::Time::GetDeltaTime();

                float gravity = 9.81f * settings.gravityScale;
                state.velocity.y -= gravity * dt;

                float maxFallSpeed = 50.0f;
                state.velocity.y = std::max(state.velocity.y, -maxFallSpeed);
            }

            void CharacterController::ApplyMovement(glm::vec3 inputDirection) {
                float dt = Lindo::Time::GetDeltaTime();
                if (dt <= 0.0f) return;

                // Нормализуем ввод
                float inputLength = glm::length(inputDirection);
                if (inputLength > 0.001f) {
                    inputDirection /= inputLength;
                }

                float maxSpeed = GetCurrentMaxSpeed();

                if (state.isGrounded) {
                    // Движение по земле
                    glm::vec3 targetVelocity = inputDirection * maxSpeed;
                    glm::vec3 velocityDiff = targetVelocity - glm::vec3(state.velocity.x, 0.0f, state.velocity.z);

                    float accelRate = settings.acceleration;
                    state.velocity.x += velocityDiff.x * accelRate * dt;
                    state.velocity.z += velocityDiff.z * accelRate * dt;

                    // Трение
                    ApplyFriction();
                }
                else {
                    // Движение в прыжке
                    if (settings.airControl > 0.0f && inputLength > 0.0f) {
                        glm::vec3 airAccel = inputDirection * maxSpeed * settings.airControl;
                        state.velocity.x += airAccel.x * dt;
                        state.velocity.z += airAccel.z * dt;
                    }
                }

                // Ограничиваем горизонтальную скорость
                glm::vec3 horizontalVel = glm::vec3(state.velocity.x, 0.0f, state.velocity.z);
                float horizontalSpeed = glm::length(horizontalVel);
                if (horizontalSpeed > maxSpeed) {
                    horizontalVel = horizontalVel * (maxSpeed / horizontalSpeed);
                    state.velocity.x = horizontalVel.x;
                    state.velocity.z = horizontalVel.z;
                }
            }

            void CharacterController::ApplyFriction() {
                float dt = Lindo::Time::GetDeltaTime();
                if (!state.isGrounded) return;

                glm::vec3 horizontalVel = glm::vec3(state.velocity.x, 0.0f, state.velocity.z);
                float speed = glm::length(horizontalVel);

                if (speed > 0.001f) {
                    float friction = settings.groundFriction;
                    float decrease = friction * dt;
                    if (decrease > speed) decrease = speed;

                    horizontalVel *= (speed - decrease) / speed;
                    state.velocity.x = horizontalVel.x;
                    state.velocity.z = horizontalVel.z;
                }
            }

            void CharacterController::HandleAutoOrientation() {
                if (!isAutoOrientationEnabled) return;

                if (orientationMode == AutoOrientationMode::MovementDirection && gameObject) {
                    if (glm::length(state.velocity) > 0.1f) {
                        float angle = atan2(state.velocity.x, state.velocity.z);
                        // gameObject->SetRotation(angle);
                    }
                }
            }

            float CharacterController::GetCurrentMaxSpeed() const {
                if (isRunningInput && movementMode == MovementMode::Walking) {
                    return settings.runSpeed;
                }

                switch (movementMode) {
                case MovementMode::Walking:  return settings.walkSpeed;
                case MovementMode::Running:  return settings.runSpeed;
                case MovementMode::Crouching: return settings.crouchSpeed;
                case MovementMode::Sliding:   return settings.slideSpeed;
                case MovementMode::Swimming:  return settings.swimSpeed;
                case MovementMode::Flying:    return settings.flySpeed;
                case MovementMode::Climbing:  return settings.climbSpeed;
                default: return settings.walkSpeed;
                }
            }

            void CharacterController::UpdateCurrentSpeed() {
                state.currentSpeed = glm::length(state.velocity);
            }

            void CharacterController::ResolveCollisions() {
                auto& physics = Lindo::Components::Physics::PhysicsSystem::GetInstance();
                bool foundGroundContact = false;
                float dt = Lindo::Time::GetDeltaTime();

                for (int pass = 0; pass < 3; ++pass) {
                    bool anyCorrection = false;

                    for (auto* other : physics.GetColliders()) {
                        if (!other || other == &collider || !other->IsEnabled()) continue;
                        if (other->gameObject == gameObject) continue;

                        Lindo::Components::Physics::CollisionInfo info;
                        if (!collider.CheckCollision(other, info)) continue;

                        glm::vec3 normal = info.contactNormal;
                        if (glm::length(normal) < 1e-6f) continue;
                        normal = glm::normalize(normal);

                        // --- ИСПРАВЛЕНИЕ: ГАРАНТИЯ ВЫТАЛКИВАНИЯ ---
                        // Защита от инверсии нормалей в Double Dispatch коллайдеров.
                        // Если нормаль указывает вглубь препятствия (толкает нас в пол), 
                        // мы её принудительно разворачиваем в сторону центра игрока.
                        glm::vec3 dirToPlayer = collider.GetWorldCenter() - other->GetWorldCenter();
                        if (glm::dot(normal, dirToPlayer) < 0.0f) {
                            normal = -normal;
                        }
                        // ------------------------------------------

                        if (normal.y > 0.5f) {
                            foundGroundContact = true;
                        }

                        float penetration = info.penetrationDepth;
                        if (penetration <= 0.0f) continue;

                        glm::vec3 correction = normal * (penetration + 0.001f);
                        glm::vec3 pushVelocity = state.velocity;

                        float velocityIntoNormal = glm::dot(state.velocity, normal);
                        if (velocityIntoNormal < 0.0f) {
                            state.velocity -= normal * velocityIntoNormal;
                        }

                        if (other->gameObject) {
                            auto* rb = other->gameObject->getComponent<Lindo::Components::Physics::RigidBody>();

                            if (rb && !rb->isKinematic) {
                                // 1. ДИНАМИЧЕСКИЙ ОБЪЕКТ: делим выталкивание по массам
                                float charMass = 80.0f;
                                float totalInvMass = (1.0f / charMass) + rb->GetInvMass();

                                if (totalInvMass > 0.0f) {
                                    float charRatio = rb->GetInvMass() / totalInvMass;
                                    float boxRatio = (1.0f / charMass) / totalInvMass;

                                    setPosition(getPosition() + correction * charRatio);
                                    rb->gameObject->transform.position -= correction * boxRatio;

                                    collider.SetOffset(glm::vec3(0.0f, collider.GetHeight() * 0.5f, 0.0f));
                                }

                                // 2. Импульс от толкания
                                glm::vec3 pushDir = pushVelocity;
                                pushDir.y = 0.0f;
                                if (glm::length(pushDir) > 0.01f) {
                                    pushDir = glm::normalize(pushDir);
                                    float pushForce = 2.0f;
                                    rb->applyImpulse(pushDir * pushForce);
                                }
                            }
                            else {
                                // Статичный объект
                                setPosition(getPosition() + correction);
                                collider.SetOffset(glm::vec3(0.0f, collider.GetHeight() * 0.5f, 0.0f));
                            }
                        }
                        else {
                            setPosition(getPosition() + correction);
                            collider.SetOffset(glm::vec3(0.0f, collider.GetHeight() * 0.5f, 0.0f));
                        }

                        anyCorrection = true;
                    }

                    if (!anyCorrection) break;
                }

                state.isGrounded = foundGroundContact;
                if (state.isGrounded) {
                    state.isJumping = false;
                    state.isSliding = false;

                    // Сбрасываем накопление падения при нахождении на земле
                    if (state.velocity.y < 0.0f) {
                        state.velocity.y = -0.1f;
                    }
                }
            }

            bool CharacterController::CheckCapsuleCollision(const Physics::CapsuleCollider& other) const {
                glm::vec3 thisPos = getPosition();
                glm::vec3 otherPos = other.getPosition();

                float distance = glm::distance(thisPos, otherPos);
                float radiusSum = collider.GetRadius() + other.GetRadius();

                return distance < radiusSum;
            }
        }
    }
}