#include "Player.h"
#include <Component/GameObject/GameObject.h>
#include <Component/PlayerController/CharacterController.h>
#include <Core/Time/Time.h>

namespace Lindo {
    namespace Components {
        namespace Controller {

            Player::Player() {
                settings = PlayerSettings();
            }

            Player::Player(const PlayerSettings& settings)
                : settings(settings) {
            }

            void Player::OnStart() {
                // Добавляем CharacterController как компонент
                characterController = gameObject->addComponent<Lindo::Components::Character::CharacterController>();

                if (characterController) {
                    // Настраиваем параметры персонажа
                    characterController->SetWalkSpeed(5.0f);
                    characterController->SetRunSpeed(10.0f);
                    characterController->SetJumpHeight(2.0f);

                    // Устанавливаем коллбеки
                    characterController->SetOnJumpCallback([this]() {
                        // Эффект качания при прыжке
                        headBobTimer = 0.5f;
                        });

                    characterController->SetOnLandCallback([this]() {
                        // Эффект при приземлении
                        headBobTimer = 0.3f;
                        });

                    characterController->SetOnCrouchCallback([this](bool crouching) {
                        // Логика переключения приседания
                        });
                }

                // Добавляем камеру как компонент
                camera = gameObject->addComponent<Lindo::Components::Rendering::Camera>();

                if (camera) {
                    // Настраиваем камеру для от первого лица
                    camera->setMode(Rendering::Camera::Mode::FirstPerson);
                    camera->setMouseSensitivity(settings.mouseSensitivity);
                    camera->invertY = settings.invertY;
                    camera->heightOffset = isCrouching ? 0.9f : 1.6f;  // Высота камеры в зависимости от приседания
                    camera->movementSpeed = 5.0f;

                    // Синхронизируем углы камеры
                    cameraYaw = camera->getYaw();
                    cameraPitch = camera->getPitch();
                }

                lastFramePosition = getPosition();
            }

            void Player::OnUpdate() {
                if (!isAlive || !characterController) return;
                if (freeCameraMode) return;
                float dt = Lindo::Time::GetDeltaTime();

                // Обновляем инвульнерабельность
                if (isInvulnerable) {
                    invulnerabilityTimer -= dt;
                    if (invulnerabilityTimer <= 0.0f) {
                        isInvulnerable = false;
                    }
                }

                ProcessInput();
                UpdateCamera();
                UpdateInputState();

                // Обновляем состояние движения для качания головы
                glm::vec3 currentPos = getPosition();
                isMoving = (glm::length(currentPos - lastFramePosition) > 0.01f);
                lastFramePosition = currentPos;

                // Обработка качания головы
                if (settings.enableHeadBob && isMoving && characterController->IsGrounded() && !isCrouching) {
                    HandleHeadBob();
                }
                else {
                    headBobTimer = 0.0f;
                }
            }

            void Player::OnDestroy() {
            }

            void Player::SetFreeCameraMode(bool enabled) {
                if (freeCameraMode == enabled || !camera || !characterController) return;
                freeCameraMode = enabled;
                characterController->SetSimulationEnabled(!enabled);
                camera->setMode(enabled ? Rendering::Camera::Mode::Free : Rendering::Camera::Mode::FirstPerson);
                forwardInput = 0.0f;
                rightInput = 0.0f;
                isRunning = false;
                if (!enabled) {
                    camera->heightOffset = isCrouching ? 0.9f : 1.6f;
                    camera->setFront(camera->getFront());
                }
            }

            void Player::ProcessInput() {
                if (!characterController) return;
                float dt = Lindo::Time::GetDeltaTime();
                // Собираем направление движения
                glm::vec3 moveDirection = glm::vec3(0.0f);

                // Получаем forward и right от камеры для движения относительно взгляда
                if (camera && camera->getMode() == Rendering::Camera::Mode::FirstPerson) {
                    glm::vec3 forward = camera->getFront();
                    glm::vec3 right = camera->getRight();

                    // Ограничиваем движение по оси Y
                    forward.y = 0.0f;
                    right.y = 0.0f;

                    forward = glm::normalize(forward);
                    right = glm::normalize(right);

                    moveDirection += forward * forwardInput;
                    moveDirection += right * rightInput;
                }
                else {
                    // Если нет камеры, используем мировые оси
                    moveDirection += glm::vec3(0.0f, 0.0f, 1.0f) * forwardInput;
                    moveDirection += glm::vec3(1.0f, 0.0f, 0.0f) * rightInput;
                }

                // Нормализуем диагональное движение
                if (glm::length(moveDirection) > 0.0f) {
                    moveDirection = glm::normalize(moveDirection);
                }

                // Применяем движение
                characterController->Move(moveDirection, isRunning);
            }

            void Player::MoveForward(float value) {
                forwardInput = value;
            }

            void Player::MoveRight(float value) {
                rightInput = value;
            }

            void Player::Look(float deltaX, float deltaY) {
                if (!camera) return;

                // Передаем движение мыши в камеру
                camera->processMouseMovement(deltaX, deltaY, true);

                // Получаем обновленные углы из камеры
                cameraYaw = camera->getYaw();
                cameraPitch = camera->getPitch();
            }

            void Player::Jump() {
                if (characterController && isAlive) {
                    characterController->Jump();
                }
            }

            void Player::StartRunning() {
                if (!isCrouching && characterController && characterController->IsGrounded()) {
                    isRunning = true;
                    if (camera) {
                        camera->movementSpeed = 10.0f; // Увеличиваем скорость камеры при беге
                    }
                }
            }

            void Player::StopRunning() {
                isRunning = false;
                if (camera) {
                    camera->movementSpeed = 5.0f; // Возвращаем обычную скорость
                }
            }

            void Player::Crouch() {
                if (characterController && !isRunning) {
                    characterController->Crouch();
                    isCrouching = true;

                    if (camera) {
                        camera->heightOffset = 0.9f; // Опускаем камеру при приседании
                    }
                }
            }

            void Player::UnCrouch() {
                if (characterController && isCrouching) {
                    // UnCrouch() может вернуть false, если сверху потолок — тогда персонаж
                    // остаётся в приседе (как в Source), и isCrouching/камеру трогать не нужно.
                    if (characterController->UnCrouch()) {
                        isCrouching = false;

                        if (camera) {
                            camera->heightOffset = 1.6f; // Поднимаем камеру обратно
                        }
                    }
                }
            }

            void Player::SetCrouchInput(bool held) {
                // Держим Ctrl — приседаем, отпустили — пытаемся встать (получится, только если
                // сверху достаточно места; иначе Player::UnCrouch() выше просто ничего не сделает
                // и мы повторим попытку в следующем кадре, пока клавиша остаётся отпущенной).
                if (held) {
                    Crouch();
                }
                else {
                    UnCrouch();
                }
            }

            void Player::Interact() {
                // TODO: Реализовать взаимодействие с объектами через Raycast
                if (camera) {
                    glm::vec3 rayOrigin = camera->getPosition();
                    glm::vec3 rayDirection = camera->getFront();
                }
            }

            void Player::TakeDamage(float damage) {
                if (!isAlive || isInvulnerable) return;

                health -= damage;

                if (health <= 0.0f) {
                    health = 0.0f;
                    isAlive = false;
                    // Эффект смерти
                }
                else {
                    // Становимся неуязвимыми на короткое время
                    isInvulnerable = true;
                    invulnerabilityTimer = 1.0f;

                    // Эффект получения урона
                    headBobTimer = 0.2f;
                }
            }

            void Player::Heal(float amount) {
                if (!isAlive) return;

                health += amount;
                if (health > 100.0f) health = 100.0f;
            }

            void Player::UpdateCamera() {
                if (!camera) return;

                SyncCameraWithCharacter();
            }

            void Player::SyncCameraWithCharacter() {
                if (!camera || !gameObject) return;
            }

            void Player::UpdateInputState() {
                // Автоматически выходим из спринта если не двигаемся
                if (isRunning && forwardInput == 0.0f && rightInput == 0.0f) {
                    StopRunning();
                }

                // Автоматически выходим из приседания при спринте
                if (isRunning && isCrouching) {
                    UnCrouch();
                }
            }

            void Player::HandleHeadBob() {
                if (!camera || !characterController) return;
                float dt = Lindo::Time::GetDeltaTime();

                // Скорость качания зависит от текущей скорости
                float speed = characterController->GetCurrentSpeed();
                float bobSpeedFactor = std::min(speed / 5.0f, 1.5f);

                headBobTimer += dt * camera->bobSpeed * bobSpeedFactor;

                // Эффект качания
                float bobY = std::sin(headBobTimer * 2.0f) * camera->bobAmount * (isRunning ? 1.5f : 1.0f);
                float bobX = std::sin(headBobTimer) * camera->bobAmount * 0.5f;
            }
        }
    }
}