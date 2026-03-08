#pragma once
#include <Component/Component.h>
#include "core/OGL.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <Component/GameObject/GameObject.h>

class Camera : public Component {
public:
    Camera() = default;
    virtual ~Camera() = default;

    // Переопределяем методы Component
    void OnStart() override;
    void OnUpdate(float deltaTime) override;

    // Обработка ввода
    void processKeyboardInput(int key, int action, float deltaTime);
    void processMouseMovement(float xOffset, float yOffset, bool constrainPitch = true);
    void processMouseScroll(float yOffset);

    // Геттеры
    glm::mat4 getViewMatrix() const;
    glm::mat4 getProjectionMatrix() const;

    // ВНИМАНИЕ: getPosition() теперь использует owner из Component
    // owner - это указатель на GameObject, который есть у всех компонентов
    glm::vec3 getPosition() const {
        return owner ? (owner->transform.position + glm::vec3(0.0f, heightOffset, 0.0f)) : glm::vec3(0.0f);
    }

    glm::vec3 getFront() const { return front; }
    glm::vec3 getUp() const { return up; }
    glm::vec3 getRight() const { return right; }
    float getZoom() const { return zoom; }
    float getYaw() const { return yaw; }
    float getPitch() const { return pitch; }

    // Сеттеры
    void setHeightOffset(float offset) { heightOffset = offset; }
    void setMovementSpeed(float speed) { movementSpeed = speed; }
    void setMouseSensitivity(float sensitivity) { mouseSensitivity = sensitivity; }
    void setWorldUp(const glm::vec3& worldUp) { this->worldUp = worldUp; updateCameraVectors(); }
    void setFront(const glm::vec3& newFront);
    void setAspectRatio(float aspect) { m_aspect = aspect; }
    void setNearFar(float nearPlane, float farPlane) { m_near = nearPlane; m_far = farPlane; }

    // Настройки
    float movementSpeed = 5.0f;
    float mouseSensitivity = 0.1f;
    float zoom = 90.0f;
    float maxPitch = 89.0f;
    float minPitch = -89.0f;
    bool invertY = false;
    float heightOffset = 1.8f;      // Высота глаз от центра объекта
    bool clampToGround = false;
    float groundHeight = 0.0f;
    float bobAmount = 0.05f;         // Амплитуда качания головы
    float bobSpeed = 10.0f;           // Скорость качания

    // Режимы камеры
    enum class Mode {
        FirstPerson,   // От первого лица (следует за игроком)
        Free           // Свободная камера
    };

    void setMode(Mode newMode) { mode = newMode; }
    Mode getMode() const { return mode; }

private:
    void updateCameraVectors();
    void updateBob(float deltaTime);

    // Состояние камеры
    glm::vec3 front = glm::vec3(0.0f, 0.0f, -1.0f);
    glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
    glm::vec3 right = glm::vec3(1.0f, 0.0f, 0.0f);
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

    // Параметры проекции
    float m_near = 0.1f;
    float m_far = 1000.0f;
    float m_aspect = 16.0f / 9.0f;

    // Эффекты
    float bobTimer = 0.0f;
    bool isBobbing = false;
    glm::vec3 bobOffset = glm::vec3(0.0f);

    // Режим работы
    Mode mode = Mode::FirstPerson;
};