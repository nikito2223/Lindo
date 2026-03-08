#include <glm/ext/vector_float3.hpp>
#include <memory>

enum class ColliderType {
    BOX,
    SPHERE,
    CAPSULE,
    MESH
};

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