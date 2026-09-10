#include "Camera.h"
#include <glm/gtc/matrix_transform.hpp>
#include "core/Types/Settings.h"
#include <iostream>
#include <Core/Time/Time.h>
#include <Core/Input.h>

namespace Lindo {
    namespace Components {
        namespace Rendering {

            void Camera::applySettings(const Lindo::Settings& settings) {
                zoom = glm::clamp(settings.fov, 1.0f, 170.0f);
                m_near = settings.nearPlane;
                m_far = settings.farPlane;
                gamma = settings.gamma;
            }

            /**
             * @brief Инициализация базовых параметров камеры на основе пользовательских настроек.
             */
            void Camera::OnStart() {
                auto& settings = Lindo::Settings::getInstance();
                applySettings(settings);

                if (gameObject) {
                    yaw = gameObject->transform.rotation.y;
                }
                updateCameraVectors();
            }

            /**
             * @brief Рассчитывает позиции свободного полета, смещения покачивания и направления.
             */
            void Camera::OnUpdate() {
                if (mode == Mode::Free) {
                    float dt = Lindo::Time::GetDeltaTime();
                    auto& input = Lindo::Input::Input::Get();
                    glm::vec3 direction(0.0f);
                    direction += front * input.getAxis("move_forward", "move_backward");
                    direction += right * input.getAxis("move_right", "move_left");
                    direction += worldUp * input.getAxis("free_up", "free_down");
                    if (glm::length(direction) > 1.0f) direction = glm::normalize(direction);

                    const float speed = movementSpeed * (input.getAction("sprint") ? 2.0f : 1.0f);
                    gameObject->transform.position += direction * speed * dt;

                    if (clampToGround && gameObject->transform.position.y < groundHeight + heightOffset)
                        gameObject->transform.position.y = groundHeight + heightOffset;
                }
                else if (mode == Mode::FirstPerson && gameObject) {
                    // В режиме от первого лица позиция определяется родительским объектом.
                }

                updateBob();
                updateCameraVectors();
            }

            /**
             * @brief Переключает состояния движения свободной камеры.
             * @param key Код клавиши GLFW.
             * @param action Тип действия над клавишей.
             */
            void Camera::processKeyboardInput(int key, int action) {
                bool pressed = (action == GLFW_PRESS || action == GLFW_REPEAT);

                switch (key) {
                case GLFW_KEY_W:
                    movementState.forward = pressed;
                    isBobbing = pressed;
                    break;
                case GLFW_KEY_S:
                    movementState.backward = pressed;
                    isBobbing = pressed;
                    break;
                case GLFW_KEY_A:
                    movementState.left = pressed;
                    isBobbing = pressed;
                    break;
                case GLFW_KEY_D:
                    movementState.right = pressed;
                    isBobbing = pressed;
                    break;
                case GLFW_KEY_SPACE:
                    movementState.up = pressed;
                    break;
                case GLFW_KEY_LEFT_SHIFT:
                    movementState.down = pressed;
                    break;
                case GLFW_KEY_LEFT_CONTROL:
                    if (pressed) movementSpeed = 10.0f;
                    else movementSpeed = 5.0f;
                    break;
                }
            }

            /**
             * @brief Рассчитывает углы поворота Yaw и Pitch на основе перемещений мыши.
             * @param xOffset Дельта смещения мыши по оси X.
             * @param yOffset Дельта смещения мыши по оси Y.
             * @param constrainPitch Ограничивать ли пределы вертикального угла.
             */
            void Camera::processMouseMovement(float xOffset, float yOffset, bool constrainPitch) {
                xOffset *= mouseSensitivity;
                yOffset *= mouseSensitivity * (invertY ? -1.0f : 1.0f);

                yaw += xOffset;
                pitch += yOffset;

                if (constrainPitch) {
                    if (pitch > maxPitch) pitch = maxPitch;
                    if (pitch < minPitch) pitch = minPitch;
                }

                if (mode == Mode::FirstPerson && gameObject) {
                    gameObject->transform.rotation.y = yaw;
                }

                updateCameraVectors();
            }

            /**
             * @brief Изменяет угол обзора камеры (FOV) в границах от 1 до 90 градусов.
             * @param yOffset Направление и дельта прокрутки колесика.
             */
            void Camera::processMouseScroll(float yOffset) {
                zoom -= yOffset;
                if (zoom < 1.0f) zoom = 1.0f;
                if (zoom > 90.0f) zoom = 90.0f;
            }

            /**
             * @brief Вычисляет итоговую матрицу взгляда с учетом всех смещений высоты и покачивания.
             * @return Матрица glm::mat4 для передачи в шейдер.
             */
            glm::mat4 Camera::getViewMatrix() const {
                glm::vec3 eyePos;

                if (gameObject) {
                    eyePos = gameObject->transform.position + glm::vec3(0.0f, heightOffset, 0.0f);
                    eyePos += bobOffset;
                }
                else {
                    eyePos = glm::vec3(0.0f);
                }

                return glm::lookAt(eyePos, eyePos + front, up);
            }

            /**
             * @brief Генерирует перспективную матрицу проекции с текущими параметрами FOV и плоскостей.
             * @return Перспективная матрица glm::mat4.
             */
            glm::mat4 Camera::getProjectionMatrix() const {
                return glm::perspective(glm::radians(zoom), m_aspect, m_near, m_far);
            }

            /**
             * @brief Пересчитывает базисные векторы (front, right, up) на основе углов Yaw и Pitch.
             */
            void Camera::updateCameraVectors() {
                glm::vec3 newFront;
                newFront.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
                newFront.y = sin(glm::radians(pitch));
                newFront.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
                front = glm::normalize(newFront);

                right = glm::normalize(glm::cross(front, worldUp));
                up = glm::normalize(glm::cross(right, front));
            }

            /**
             * @brief Обновляет смещение эффекта покачивания головы при ходьбе.
             */
            void Camera::updateBob() {
                float dt = Lindo::Time::GetDeltaTime();

                if (isBobbing && (movementState.forward || movementState.backward ||
                    movementState.left || movementState.right)) {
                    bobTimer += dt * bobSpeed;

                    float bobX = sin(bobTimer * 2.0f) * bobAmount;
                    float bobY = abs(sin(bobTimer)) * bobAmount * 0.5f;

                    bobOffset = glm::vec3(bobX, bobY, 0.0f);
                }
                else {
                    bobOffset = glm::mix(bobOffset, glm::vec3(0.0f), dt * 5.0f);
                    bobTimer = 0.0f;
                }
            }

            /**
             * @brief Задает новое направление взгляда камеры и обновляет углы.
             * @param newFront Нормализованный вектор направления.
             */
            void Camera::setFront(const glm::vec3& newFront) {
                front = glm::normalize(newFront);
                float clampedY = glm::clamp(front.y, -1.0f, 1.0f);
                pitch = glm::degrees(asin(clampedY));
                yaw = glm::degrees(atan2(front.z, front.x));
                updateCameraVectors();
            }
        }
    }
}