#include "Camera.h"
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

void Camera::OnStart() {
    // Инициализация
    if (owner) {
        yaw = owner->transform.rotation.y;
    }
    updateCameraVectors();
}

void Camera::OnUpdate(float deltaTime) {
    if (mode == Mode::Free) {
        // Свободное движение камеры
        float velocity = movementSpeed * deltaTime;
        if (movementState.forward)  owner->transform.position += front * velocity;
        if (movementState.backward) owner->transform.position -= front * velocity;
        if (movementState.left)     owner->transform.position -= right * velocity;
        if (movementState.right)    owner->transform.position += right * velocity;
        if (movementState.up)       owner->transform.position += worldUp * velocity;
        if (movementState.down)     owner->transform.position -= worldUp * velocity;

        if (clampToGround && owner->transform.position.y < groundHeight + heightOffset)
            owner->transform.position.y = groundHeight + heightOffset;
    }
    else if (mode == Mode::FirstPerson && owner) {
        // В режиме от первого лица камера следует за позицией owner
        // Позиция owner может меняться физsикой или движением игрока
        // Ничего не делаем, просто обновляем векторы
    }

    // Обновляем эффект качания
    updateBob(deltaTime);

    // Визуальное смещение от эффектов (не влияет на коллизии)
    // position += bobOffset; - теперь это делается в getViewMatrix()

    updateCameraVectors();
}

void Camera::processKeyboardInput(int key, int action, float deltaTime) {
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

    // Ограничиваем угол наклона
    if (constrainPitch) {
        if (pitch > maxPitch) pitch = maxPitch;
        if (pitch < minPitch) pitch = minPitch;
    }

    // В режиме от первого лица поворачиваем игрока вместе с камерой
    if (mode == Mode::FirstPerson && owner) {
        owner->transform.rotation.y = yaw;
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

    if (owner) {
        // Базовая позиция - центр объекта + смещение по высоте
        eyePos = owner->transform.position + glm::vec3(0.0f, heightOffset, 0.0f);

        // Добавляем эффект качания (только визуально)
        eyePos += bobOffset;
    }
    else {
        eyePos = glm::vec3(0.0f); // Запасной вариант
    }

    return glm::lookAt(eyePos, eyePos + front, up);
}

glm::mat4 Camera::getProjectionMatrix() const {
    return glm::perspective(glm::radians(zoom), m_aspect, m_near, m_far);
}

void Camera::updateCameraVectors() {
    // Вычисляем новый вектор фронта
    glm::vec3 newFront;
    newFront.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    newFront.y = sin(glm::radians(pitch));
    newFront.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    front = glm::normalize(newFront);

    // Пересчитываем право и вверх
    right = glm::normalize(glm::cross(front, worldUp));
    up = glm::normalize(glm::cross(right, front));
}

void Camera::updateBob(float deltaTime) {
    if (isBobbing && (movementState.forward || movementState.backward ||
        movementState.left || movementState.right)) {
        bobTimer += deltaTime * bobSpeed;

        // Простой синусоидальный боббинг
        float bobX = sin(bobTimer * 2.0f) * bobAmount;
        float bobY = abs(sin(bobTimer)) * bobAmount * 0.5f;

        bobOffset = glm::vec3(bobX, bobY, 0.0f);
    }
    else {
        // Плавное возвращение в исходное положение
        bobOffset = glm::mix(bobOffset, glm::vec3(0.0f), deltaTime * 5.0f);
        bobTimer = 0.0f;
    }
}

void Camera::setFront(const glm::vec3& newFront) {
    front = glm::normalize(newFront);
    pitch = glm::degrees(asin(front.y));
    yaw = glm::degrees(atan2(front.z, front.x));
    updateCameraVectors();
}