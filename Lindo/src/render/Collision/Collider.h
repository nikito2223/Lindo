// Collider.h
#pragma once

#include "render/mesh/Transform.h"
#include <camera/CameraSettings.cpp>

// Предварительное объявление
class Shader;

// Базовый класс коллайдера
class Collider {
public:
    Collider(ColliderType type, const Transform& transform = Transform())
        : type(type), transform(transform), isTrigger(false), isVisible(true),
        debugColor(glm::vec3(0.0f, 1.0f, 0.0f)) {}

    virtual ~Collider() = default;

    // Виртуальные методы для проверки столкновений
    virtual bool checkCollision(const std::shared_ptr<Collider>& other,
        CollisionInfo* info = nullptr) const {
        return checkCollision(other.get(), info);
    }

    virtual bool checkCollision(const Collider* other,
        CollisionInfo* info = nullptr) const = 0;

    virtual bool checkRayCollision(const glm::vec3& origin, const glm::vec3& direction,
        float* distance = nullptr, glm::vec3* normal = nullptr) const = 0;

    // Виртуальные методы для отладочной визуализации
    virtual void drawDebug(Shader& shader) const = 0;

    virtual glm::vec3 getCenter() const = 0;
    virtual glm::vec3 getExtents() const = 0;

    // Геттеры и сеттеры
    ColliderType getType() const { return type; }
    Transform& getTransform() { return transform; }
    const Transform& getTransform() const { return transform; }

    void setTransform(const Transform& t) { transform = t; }
    void setVisible(bool visible) { isVisible = visible; }
    bool getVisible() const { return isVisible; }

    void setDebugColor(const glm::vec3& color) { debugColor = color; }
    glm::vec3 getDebugColor() const { return debugColor; }

    void setTrigger(bool trigger) { isTrigger = trigger; }
    bool getTrigger() const { return isTrigger; }

    void setUserData(void* data) { userData = data; }
    void* getUserData() const { return userData; }

protected:
    ColliderType type;
    Transform transform;
    bool isTrigger;
    bool isVisible;
    glm::vec3 debugColor;
    void* userData = nullptr;
};