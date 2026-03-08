// BoxCollider.h
#pragma once

#include "Collider.h"
#include <vector>
#include <Component/Component.h>
#include <Component/GameObject/GameObject.h>

class BoxCollider : public Collider {
public:
    BoxCollider(const glm::vec3& size = glm::vec3(1.0f))
        : Collider(ColliderType::BOX), size(size) {
        generateDebugMesh();
    }

    BoxCollider(float width, float height, float depth,
        const Transform& transform = Transform())
        : Collider(ColliderType::BOX),
        size(glm::vec3(width, height, depth)) {
        generateDebugMesh();
    }

    bool BoxCollider::intersectRay(const glm::vec3& origin,
        const glm::vec3& direction,
        float maxDistance,
        RaycastHit& hit) const {
        float distance;
        glm::vec3 normal;
        if (checkRayCollision(origin, direction, &distance, &normal)) {
            if (distance <= maxDistance) {
                hit.distance = distance;
                hit.point = origin + direction * distance;
                hit.normal = normal;
                return true;
            }
        }
        return false;
    }

    // Проверка столкновений
    bool checkCollision(const Collider* other, CollisionInfo* info = nullptr) const override;
    bool checkRayCollision(const glm::vec3& origin, const glm::vec3& direction,
        float* distance = nullptr, glm::vec3* normal = nullptr) const override;

    // Отладочная визуализация
    void drawDebug(Shader& shader) const override;

    // Геометрические данные
    glm::vec3 getCenter() const override {
        return getPosition();
    }

    glm::vec3 getExtents() const override {
        return size * 0.5f;
    }

    glm::vec3 getHalfSize() const { return size * 0.5f; }
    glm::vec3 getSize() const { return size; }
    void setSize(const glm::vec3& newSize) {
        size = newSize;
        generateDebugMesh();
    }

    // Получение вершин в мировых координатах
    std::vector<glm::vec3> getWorldVertices() const;

private:
    void generateDebugMesh() const;

    glm::vec3 size;

    // Для отладочной отрисовки
    mutable unsigned int VAO = 0, VBO = 0, EBO = 0;
    mutable std::vector<float> debugVertices;
    mutable std::vector<unsigned int> debugIndices;
};