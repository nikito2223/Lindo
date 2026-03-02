#pragma once

#include "Collider.h"
#include <vector>
#include <glm/glm.hpp>
#include <Graphics/core/Shader.h>

// Предварительные объявления (чтобы избежать циклических зависимостей)
class BoxCollider;
struct CollisionInfo; // если не определён в Collider.h

class CapsuleCollider : public Collider {
public:
    CapsuleCollider(float radius = 0.5f, float height = 2.0f,
        const Transform& transform = Transform())
        : Collider(ColliderType::CAPSULE, transform),
        radius(radius), height(height) {
        generateDebugMesh();
    }

    bool intersectRay(const glm::vec3& origin,
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

    bool checkCollision(const Collider* other, CollisionInfo* info = nullptr) const override;
    bool checkRayCollision(const glm::vec3& origin, const glm::vec3& direction,
        float* distance = nullptr, glm::vec3* normal = nullptr) const override;

    glm::vec3 getCenter() const override {
        return transform.position;
    }

    glm::vec3 getExtents() const override {
        return glm::vec3(radius, height * 0.5f + radius, radius);
    }

    float getRadius() const { return radius; }
    float getHeight() const { return height; }

    void setRadius(float newRadius) {
        radius = newRadius;
        generateDebugMesh();
    }

    void setHeight(float newHeight) {
        height = newHeight;
        generateDebugMesh();
    }


    bool checkCollisionWithBox(const BoxCollider* box, CollisionInfo* info) const;
    void drawDebug(Shader& shader) const override;

    glm::vec3 getTopSphereCenter() const;
    glm::vec3 getBottomSphereCenter() const;

private:
    void generateDebugMesh() const;

    float radius;
    float height;

    mutable unsigned int VAO = 0, VBO = 0, EBO = 0;
    mutable std::vector<float> debugVertices;
    mutable std::vector<unsigned int> debugIndices;
};