#pragma once

#include "Collider.h"
#include <shader/shader_s.h>

class SphereCollider : public Collider {
public:
    SphereCollider(float radius = 1.0f,
        const Transform& transform = Transform())
        : Collider(ColliderType::SPHERE, transform), radius(radius) {
        generateDebugMesh();
    }

    // Проверка столкновений
    bool checkCollision(const Collider* other, CollisionInfo* info = nullptr) const override;
    bool checkRayCollision(const glm::vec3& origin, const glm::vec3& direction,
        float* distance = nullptr, glm::vec3* normal = nullptr) const override;

    // Геометрические данные
    glm::vec3 getCenter() const override {
        return transform.position;
    }

    glm::vec3 getExtents() const override {
        return glm::vec3(radius, radius, radius);
    }

    float getRadius() const { return radius; }
    void setRadius(float newRadius) {
        radius = newRadius;
        generateDebugMesh();
    }

    void drawDebug(Shader& shader) const override;

private:

    void generateDebugMesh() const;

    float radius;

    // Для отладочной отрисовки
    mutable unsigned int VAO = 0, VBO = 0, EBO = 0;
    mutable std::vector<float> debugVertices;
    mutable std::vector<unsigned int> debugIndices;
};