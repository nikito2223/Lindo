#include "Camera.h"
#include <glm/gtc/matrix_transform.hpp>
#include "core/Types/Settings.h"
#include <iostream>
#include <Core/Time/Time.h>

namespace Lindo {
    namespace Components {
        namespace Rendering {

            void Camera::OnStart() {
                auto& settings = Lindo::Settings::getInstance();

                // Подтягиваем параметры из глобальных настроек
                zoom = settings.fov;
                m_near = settings.nearPlane;
                m_far = settings.farPlane;
                gamma = settings.gamma;

                if (gameObject) {
                    yaw = gameObject->transform.rotation.y;
                }
                updateCameraVectors();
            }

            void Camera::OnUpdate() {
                if (mode == Mode::Free) {
                    float dt = Lindo::Time::GetDeltaTime();

                    float velocity = movementSpeed * dt;
                    if (movementState.forward)  gameObject->transform.position += front * velocity;
                    if (movementState.backward) gameObject->transform.position -= front * velocity;
                    if (movementState.left)     gameObject->transform.position -= right * velocity;
                    if (movementState.right)    gameObject->transform.position += right * velocity;
                    if (movementState.up)       gameObject->transform.position += worldUp * velocity;
                    if (movementState.down)     gameObject->transform.position -= worldUp * velocity;

                    if (clampToGround && gameObject->transform.position.y < groundHeight + heightOffset)
                        gameObject->transform.position.y = groundHeight + heightOffset;
                }
                else if (mode == Mode::FirstPerson && gameObject) {
                    // � ������ �� ������� ���� ������ ������� �� �������� gameObject
                    // ������� gameObject ����� �������� ���s���� ��� ��������� ������
                    // ������ �� ������, ������ ��������� �������
                }

                // ��������� ������ �������
                updateBob();

                // ���������� �������� �� �������� (�� ������ �� ��������)
                // position += bobOffset; - ������ ��� �������� � getViewMatrix()

                updateCameraVectors();
            }

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

            void Camera::processMouseMovement(float xOffset, float yOffset, bool constrainPitch) {
                xOffset *= mouseSensitivity;
                yOffset *= mouseSensitivity * (invertY ? -1.0f : 1.0f);

                yaw += xOffset;
                pitch += yOffset;

                // ������������ ���� �������
                if (constrainPitch) {
                    if (pitch > maxPitch) pitch = maxPitch;
                    if (pitch < minPitch) pitch = minPitch;
                }

                // � ������ �� ������� ���� ������������ ������ ������ � �������
                if (mode == Mode::FirstPerson && gameObject) {
                    gameObject->transform.rotation.y = yaw;
                }

                updateCameraVectors();
            }

            void Camera::processMouseScroll(float yOffset) {
                zoom -= yOffset;
                if (zoom < 1.0f) zoom = 1.0f;
                if (zoom > 90.0f) zoom = 90.0f;
            }

            glm::mat4 Camera::getViewMatrix() const {
                glm::vec3 eyePos;

                if (gameObject) {
                    // ������� ������� - ����� ������� + �������� �� ������
                    eyePos = gameObject->transform.position + glm::vec3(0.0f, heightOffset, 0.0f);

                    // ��������� ������ ������� (������ ���������)
                    eyePos += bobOffset;
                }
                else {
                    eyePos = glm::vec3(0.0f); // �������� �������
                }

                return glm::lookAt(eyePos, eyePos + front, up);
            }

            glm::mat4 Camera::getProjectionMatrix() const {
                return glm::perspective(glm::radians(zoom), m_aspect, m_near, m_far);
            }

            void Camera::updateCameraVectors() {
                // ��������� ����� ������ ������
                glm::vec3 newFront;
                newFront.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
                newFront.y = sin(glm::radians(pitch));
                newFront.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
                front = glm::normalize(newFront);

                // ������������� ����� � �����
                right = glm::normalize(glm::cross(front, worldUp));
                up = glm::normalize(glm::cross(right, front));
            }


            void Camera::updateBob() {
                float dt = Lindo::Time::GetDeltaTime();

                if (isBobbing && (movementState.forward || movementState.backward ||
                    movementState.left || movementState.right)) {
                    bobTimer += dt * bobSpeed;

                    // ������� �������������� �������
                    float bobX = sin(bobTimer * 2.0f) * bobAmount;
                    float bobY = abs(sin(bobTimer)) * bobAmount * 0.5f;

                    bobOffset = glm::vec3(bobX, bobY, 0.0f);
                }
                else {
                    // ������� ����������� � �������� ���������
                    bobOffset = glm::mix(bobOffset, glm::vec3(0.0f), dt * 5.0f);
                    bobTimer = 0.0f;
                }
            }

            void Camera::setFront(const glm::vec3& newFront) {
                front = glm::normalize(newFront);
                float clampedY = glm::clamp(front.y, -1.0f, 1.0f); // Защита от NaN
                pitch = glm::degrees(asin(clampedY));
                yaw = glm::degrees(atan2(front.z, front.x));
                updateCameraVectors();
            }
        }
    }
}