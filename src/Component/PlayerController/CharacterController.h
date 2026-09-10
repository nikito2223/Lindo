#pragma once
#include <functional>
#include "Component/Component.h"
#include <Component/Physhcs/Colliders/CapsuleCollider.h>

namespace Lindo {
    namespace Components {
        namespace Character {
            enum class MovementMode {
                Walking,    // ������� ������
                Running,    // ���
                Crouching,  // ����������
                Sliding,    // ����������
                Falling,    // �������
                Flying,     // �����
                Swimming,   // ��������
                Climbing    // �������
            };

            enum class AutoOrientationMode {
                None,
                MovementDirection,
                CameraDirection,
                Velocity
            };

            struct CharacterMovementSettings {
                float walkSpeed = 5.0f;
                float runSpeed = 10.0f;
                float crouchSpeed = 2.5f;
                float slideSpeed = 15.0f;
                float swimSpeed = 3.0f;
                float flySpeed = 12.0f;
                float climbSpeed = 3.0f;

                float acceleration = 10.0f;      // sv_accelerate — разгон на земле к wishSpeed
                float deceleration = 8.0f;        // не используется движком трения напрямую, оставлено для совместимости
                float groundFriction = 8.0f;      // sv_friction
                float stopSpeed = 2.0f;           // sv_stopspeed — порог для трения на малых скоростях
                float airControl = 0.3f;          // не используется новой моделью, оставлено для совместимости
                float airAccelerate = 10.0f;      // sv_airaccelerate — разгон в воздухе
                float airWishSpeedCap = 2.0f;      // потолок wishSpeed, до которого воздушный разгон "цепляет" скорость (в Source ~30u/s), выше — страйф всё ещё разгоняет, но уже не за счёт клэмпа
                float gravityScale = 1.0f;
                float jumpHeight = 2.0f;
                float maxStepHeight = 0.45f;
                float slopeLimit = 45.0f;

                bool enableAutoStep = true;
                bool maintainAirControl = false;
            };

            struct MovementState {
                glm::vec3 velocity = glm::vec3(0.0f);
                glm::vec3 lastVelocity = glm::vec3(0.0f);
                glm::vec3 acceleration = glm::vec3(0.0f);
                glm::vec3 groundNormal = glm::vec3(0.0f, 1.0f, 0.0f);
                bool isGrounded = false;
                bool wasGrounded = false;
                bool isJumping = false;
                bool isCrouching = false;
                bool isSliding = false;
                float currentSpeed = 0.0f;
            };

            class CharacterController : public Lindo::World::Component {
            public:
                CharacterController();

                // 2. ������������ ����������� � �����������
                explicit CharacterController(const CharacterMovementSettings& settings);

                virtual ~CharacterController() = default;

                // Component overrides
                void OnStart() override;
                void OnUpdate() override;
                void OnDestroy() override;

                // �������� ������ ��������
                void Move(glm::vec3 inputDirection, bool isRunning = false);
                void Rotate(glm::vec3 lookDirection);
                bool Jump();
                bool Crouch();
                bool UnCrouch();
                // Вызывается каждый кадр из системы ввода с текущим состоянием клавиши приседания
                // (например Ctrl), как в Source: держим — приседаем, отпустили — встаём (если есть место).
                void SetCrouchHeld(bool held);
                bool IsCrouchHeld() const { return crouchHeld; }
                bool Slide();
                void StopMovement();
                void Teleport(const glm::vec3& position);

                // ����������
                void SetMovementMode(MovementMode mode);
                void SetAutoOrientationMode(AutoOrientationMode mode);
                void AddImpulse(const glm::vec3& impulse);
                void SetVelocity(const glm::vec3& newVelocity);

                // �������
                void SetMovementSettings(const CharacterMovementSettings& settings);
                void SetWalkSpeed(float speed) { settings.walkSpeed = speed; }
                void SetRunSpeed(float speed) { settings.runSpeed = speed; }
                void SetJumpHeight(float height) { settings.jumpHeight = height; }
                void SetSimulationEnabled(bool enabled) { m_simulationEnabled = enabled; }
                bool IsSimulationEnabled() const { return m_simulationEnabled; }

                // �������
                bool IsGrounded() const { return state.isGrounded; }
                bool IsMoving() const { return glm::length(state.velocity) > 0.1f; }
                float GetCurrentSpeed() const { return state.currentSpeed; }
                glm::vec3 GetVelocity() const { return state.velocity; }
                MovementMode GetMovementMode() const { return movementMode; }
                Physics::CapsuleCollider& GetCollider() { return collider; }

                // �������
                void SetOnJumpCallback(std::function<void()> callback) { onJump = callback; }
                void SetOnLandCallback(std::function<void()> callback) { onLand = callback; }
                void SetOnCrouchCallback(std::function<void(bool)> callback) { onCrouch = callback; }

            private:
                void UpdateGroundStatus();
                void UpdatePhysics();
                void ApplyGravity();
                void ApplyMovement(glm::vec3 inputDirection);
                void ApplyFriction();
                // Source-style разгон: сначала считаем добавочную скорость вдоль wishDir, затем клэмпим её.
                void GroundAccelerate(const glm::vec3& wishDir, float wishSpeed, float accel);
                void AirAccelerate(const glm::vec3& wishDir, float wishSpeed, float accel);
                // Проверка, есть ли место над головой, чтобы встать из приседа (нет потолка).
                bool CanStandUp() const;
                void HandleAutoOrientation();
                float GetCurrentMaxSpeed() const;
                void UpdateCurrentSpeed();
                void ResolveCollisions();
                bool CheckCapsuleCollision(const Physics::CapsuleCollider& other) const;

                // ����������
                MovementState state;
                CharacterMovementSettings settings;
                MovementMode movementMode = MovementMode::Walking;
                AutoOrientationMode orientationMode = AutoOrientationMode::MovementDirection;
                Physics::CapsuleCollider collider;

                // ����
                glm::vec3 currentInput = glm::vec3(0.0f);
                bool isRunningInput = false;
                bool crouchHeld = false;
                float jumpBufferTimer = 0.0f;
                bool jumpRequested = false;

                // �������
                std::function<void()> onJump;
                bool m_simulationEnabled = true;
                std::function<void()> onLand;
                std::function<void(bool)> onCrouch;

                // ���������
                glm::vec3 lastSafePosition;
                float originalCrouchHeight = 1.2f;
                float originalHeight = 1.8f;
                bool isAutoOrientationEnabled = true;
            };
        }
    }
}