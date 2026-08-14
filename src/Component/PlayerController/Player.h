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
                // Настройки игрока
                struct PlayerSettings {
                    std::string playerName = "Player";
                    float mouseSensitivity = 0.1f;
                    bool invertY = false;
                    bool enableHeadBob = true;     // Включить качание головы
                };

                Player();
                explicit Player(const PlayerSettings& settings);

                // Component overrides
                void OnStart() override;
                void OnUpdate(float deltaTime) override;
                void OnDestroy() override;

                // Управление
                void ProcessInput(float deltaTime);
                void MoveForward(float value);
                void MoveRight(float value);
                void Look(float deltaX, float deltaY);

                // Действия
                void Jump();
                void StartRunning();
                void StopRunning();
                void Crouch();
                void UnCrouch();
                void Interact();

                // Геттеры/сеттеры
                void SetPlayerName(const std::string& name) { settings.playerName = name; }
                std::string GetPlayerName() const { return settings.playerName; }
                Lindo::Components::Character::CharacterController* GetCharacterController() const { return characterController; }
                Rendering::Camera* GetCamera() const { return camera; }

                // Состояние
                bool IsAlive() const { return isAlive; }
                void SetAlive(bool alive) { isAlive = alive; }
                float GetHealth() const { return health; }
                void TakeDamage(float damage);
                void Heal(float amount);

            private:
                void UpdateCamera();
                void SyncCameraWithCharacter();
                void UpdateInputState();
                void HandleHeadBob(float deltaTime);

                // Компоненты
                Lindo::Components::Character::CharacterController* characterController = nullptr;
                Rendering::Camera* camera = nullptr;

                // Настройки
                PlayerSettings settings;

                // Состояние
                bool isAlive = true;
                float health = 100.0f;

                // Ввод
                float forwardInput = 0.0f;
                float rightInput = 0.0f;
                float lastMouseX = 0.0f;
                float lastMouseY = 0.0f;
                bool isRunning = false;
                bool isCrouching = false;
                bool isMoving = false;

                // Камера
                float cameraYaw = -90.0f;  // Инициализируем как в камере
                float cameraPitch = 0.0f;

                // Вспомогательные
                glm::vec3 lastFramePosition;
                float invulnerabilityTimer = 0.0f;
                bool isInvulnerable = false;
                float headBobTimer = 0.0f;
            };
        }
    }
}