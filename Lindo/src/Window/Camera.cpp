// FirstPersonCamera.cpp
#include "Camera.h"
#include <iostream>

Camera::Camera(const Transform& playerTransform)
    : playerTransform(const_cast<Transform*>(&playerTransform)), isAttachedToPlayer(true) {

    // Начальная позиция - над игроком
    position = playerTransform.position + glm::vec3(0.0f, heightOffset, 0.0f);
    updateCameraVectors();
}

Camera::Camera(const glm::vec3& startPosition)
    : position(startPosition), isAttachedToPlayer(false) {

    updateCameraVectors();
}

void Camera::update(float deltaTime) {
    if (isAttachedToPlayer && playerTransform) {
        // Позиция = позиция игрока + смещение по высоте
        position = playerTransform->position + glm::vec3(0.0f, heightOffset, 0.0f);
    }
    else {
        // Свободное движение (как было)
        float velocity = movementSpeed * deltaTime;
        if (movementState.forward)  position += front * velocity;
        if (movementState.backward) position -= front * velocity;
        if (movementState.left)     position -= right * velocity;
        if (movementState.right)    position += right * velocity;
        if (movementState.up)       position += worldUp * velocity;
        if (movementState.down)     position -= worldUp * velocity;
        if (clampToGround && position.y < groundHeight + heightOffset)
            position.y = groundHeight + heightOffset;
    }

    updateBob(deltaTime);
    position += bobOffset;
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
        if (pitch > maxPitch)
            pitch = maxPitch;
        if (pitch < minPitch)
            pitch = minPitch;
    }

    // Обновляем вращение игрока, если камера привязана
    if (isAttachedToPlayer && playerTransform) {
        glm::vec3 oldPos = playerTransform->position;
        playerTransform->rotation.y = yaw;
        glm::vec3 newPos = playerTransform->position;
        if (oldPos != newPos) {
            std::cout << "Position changed from ("
                << oldPos.x << ", " << oldPos.y << ", " << oldPos.z
                << ") to ("
                << newPos.x << ", " << newPos.y << ", " << newPos.z
                << ")\n";
        }
    }



    updateCameraVectors();
}

void Camera::processMouseScroll(float yOffset) {
    zoom -= yOffset;
    if (zoom < 1.0f)
        zoom = 1.0f;
    if (zoom > 90.0f)
        zoom = 90.0f;
}

glm::mat4 Camera::getViewMatrix() const {
    return glm::lookAt(position, position + front, up);
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

void Camera::setPlayerTransform(Transform& transform) {
    playerTransform = &transform;
    isAttachedToPlayer = true;
    position = transform.position + glm::vec3(0.0f, heightOffset, 0.0f);
    yaw = transform.rotation.y;
}

void Camera::setPosition(const glm::vec3& newPosition) {
    position = newPosition;
    if (isAttachedToPlayer) {
        isAttachedToPlayer = false;
        playerTransform = nullptr;
    }
}
