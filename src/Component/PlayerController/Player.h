#pragma once
#include <Component/Component.h>
#include <Component/Camera/Camera.h>
#include <string>

namespace Lindo {
    namespace Components {
        namespace Character {
            class CharacterController;
        }
    }
}

namespace Lindo {
    namespace Components {
        namespace Controller {
            class Player : public Lindo::World::Component {
            public:
                // ��������� ������
                struct PlayerSettings {
                    std::string playerName = "Player";
                    float mouseSensitivity = 0.1f;
                    bool invertY = false;
                    bool enableHeadBob = true;     // �������� ������� ������
                };

                Player();
                explicit Player(const PlayerSettings& settings);

                // Component overrides
                void OnStart() override;
                void OnUpdate() override;
                void OnDestroy() override;

                // ����������
                void ProcessInput();
                void MoveForward(float value);
                void MoveRight(float value);
                void Look(float deltaX, float deltaY);

                // ��������
                void Jump();
                void StartRunning();
                void StopRunning();
                void Crouch();
                void UnCrouch();
                // Вызывать каждый кадр из системы ввода с текущим состоянием клавиши (например LeftControl):
                // held == true, пока клавиша зажата, false — как только отпущена.
                void SetCrouchInput(bool held);
                void SetFreeCameraMode(bool enabled);
                bool IsFreeCameraMode() const { return freeCameraMode; }
                void Interact();

                // �������/�������
                void SetPlayerName(const std::string& name) { settings.playerName = name; }
                std::string GetPlayerName() const { return settings.playerName; }
                Lindo::Components::Character::CharacterController* GetCharacterController() const { return characterController; }
                Rendering::Camera* GetCamera() const { return camera; }

                // ���������
                bool IsAlive() const { return isAlive; }
                void SetAlive(bool alive) { isAlive = alive; }
                float GetHealth() const { return health; }
                void TakeDamage(float damage);
                void Heal(float amount);

            private:
                void UpdateCamera();
                void SyncCameraWithCharacter();
                void UpdateInputState();
                void HandleHeadBob();

                // ����������
                Lindo::Components::Character::CharacterController* characterController = nullptr;
                Rendering::Camera* camera = nullptr;

                // ���������
                PlayerSettings settings;

                // ���������
                bool isAlive = true;
                float health = 100.0f;

                // ����
                float forwardInput = 0.0f;
                float rightInput = 0.0f;
                float lastMouseX = 0.0f;
                float lastMouseY = 0.0f;
                bool isRunning = false;
                bool isCrouching = false;
                bool isMoving = false;

                // ������
                float cameraYaw = -90.0f;  // �������������� ��� � ������
                float cameraPitch = 0.0f;

                // ���������������
                glm::vec3 lastFramePosition;
                float invulnerabilityTimer = 0.0f;
                bool isInvulnerable = false;
                float headBobTimer = 0.0f;
                bool freeCameraMode = false;
            };
        }
    }
}