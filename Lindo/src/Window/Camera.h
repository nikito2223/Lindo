// FirstPersonCamera.h
#pragma once

#include "core/OGL.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <objects/transform.h>

class Camera {
public:
    // Конструкторы
    Camera(const Transform& playerTransform);
    Camera(const glm::vec3& startPosition = glm::vec3(0.0f, 0.0f, 3.0f));

    // Основные методы
    void update(float deltaTime);
    void processKeyboardInput(int key, int action, float deltaTime);
    void processMouseMovement(float xOffset, float yOffset, bool constrainPitch = true);
    void processMouseScroll(float yOffset);

    // Настройки
    float movementSpeed = 5.0f;
    float mouseSensitivity = 0.1f;
    float zoom = 90.0f;
    float maxPitch = 89.0f;
    float minPitch = -89.0f;
    bool invertY = false;
    float heightOffset = 0.0f; // Высота глаз от земли (стандарт)
    bool clampToGround = false;
    float groundHeight = 0.0f;
    float bobAmount = 0.05f;    // Амплитуда качания головы
    float bobSpeed = 10.0f;     // Скорость качания

    // Геттеры
    glm::mat4 getViewMatrix() const;
    glm::vec3 getPosition() const { return position; }
    glm::vec3 getFront() const { return front; }
    glm::vec3 getUp() const { return up; }
    glm::vec3 getRight() const { return right; }
    float getZoom() const { return zoom; }
    float getYaw() const { return yaw; }
    float getPitch() const { return pitch; }

    // Сеттеры
    void setHeightOffset(float offset) { heightOffset = offset; }
    void setPlayerTransform(Transform& transform);  // Убрать const
    void setPosition(const glm::vec3& newPosition);
    void setMovementSpeed(float speed) { movementSpeed = speed; }
    void setMouseSensitivity(float sensitivity) { mouseSensitivity = sensitivity; }
    void setWorldUp(const glm::vec3& worldUp) { this->worldUp = worldUp; updateCameraVectors(); }
    void setFront(const glm::vec3& newFront)
    {
        front = glm::normalize(newFront);
        pitch = glm::degrees(asin(front.y));
        yaw = glm::degrees(atan2(front.z, front.x));
        updateCameraVectors();
    }


private:
    void updateCameraVectors();
    void updateBob(float deltaTime);

    // Состояние камеры
    glm::vec3 position;
    glm::vec3 front;
    glm::vec3 up;
    glm::vec3 right;
    glm::vec3 worldUp = glm::vec3(0.0f, 1.0f, 0.0f);

    // Углы Эйлера
    float yaw = -90.0f;
    float pitch = 0.0f;

    // Состояние ввода
    struct {
        bool forward = false;
        bool backward = false;
        bool left = false;
        bool right = false;
        bool up = false;
        bool down = false;
    } movementState;

    // Привязка к игроку (неконстантная ссылка)
    Transform* playerTransform = nullptr;
    bool isAttachedToPlayer = false;

    // Эффекты
    float bobTimer = 0.0f;
    bool isBobbing = false;
    glm::vec3 bobOffset = glm::vec3(0.0f);
};