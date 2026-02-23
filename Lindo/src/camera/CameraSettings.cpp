#pragma once

#include <glm/glm.hpp>
#include <memory>

// Типы коллайдеров
enum class ColliderType {
    BOX,
    SPHERE,
    CAPSULE,
    MESH
};

// Структура для хранения информации о столкновении
struct CollisionInfo {

    float penetration;       // Проникновение/глубина столкновения
    glm::vec3 point;         // Точка столкновения

    bool hasCollision = false;
    glm::vec3 normal;           // Нормаль столкновения
    float depth = 0.0f;         // Глубина проникновения
    glm::vec3 contactPoint;     // Точка контакта
    bool trigger = false;       // Если это триггер (не физика)
    void* userData = nullptr;   // Пользовательские данные
};