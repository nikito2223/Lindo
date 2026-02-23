// Frustum.h
#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <array>

class Frustum {
public:
    enum Plane {
        LEFT = 0,
        RIGHT,
        BOTTOM,
        TOP,
        NEAR,
        FAR,
        COUNT
    };

    struct PlaneData {
        glm::vec3 normal;
        float distance;

        PlaneData() : normal(0.0f), distance(0.0f) {}
        PlaneData(const glm::vec3& n, const glm::vec3& point) {
            normal = glm::normalize(n);
            distance = glm::dot(normal, point);
        }

        float distanceToPoint(const glm::vec3& point) const {
            return glm::dot(normal, point) - distance;
        }
    };

    Frustum() = default;

    void update(const glm::mat4& viewProjectionMatrix);

    bool isBoxVisible(const glm::vec3& minPoint, const glm::vec3& maxPoint) const;
    bool isSphereVisible(const glm::vec3& center, float radius) const;
    bool isPointVisible(const glm::vec3& point) const;

private:
    std::array<PlaneData, COUNT> planes;

    void normalizePlane(PlaneData& plane);
};