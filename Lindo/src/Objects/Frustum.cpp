// Frustum.cpp
#include "Frustum.h"
#include <glm/gtc/type_ptr.hpp>

void Frustum::update(const glm::mat4& viewProjectionMatrix) {
    const float* m = glm::value_ptr(viewProjectionMatrix);

    // Извлекаем плоскости из матрицы проекции
    // Левая плоскость
    planes[LEFT].normal.x = m[3] + m[0];
    planes[LEFT].normal.y = m[7] + m[4];
    planes[LEFT].normal.z = m[11] + m[8];
    planes[LEFT].distance = m[15] + m[12];

    // Правая плоскость
    planes[RIGHT].normal.x = m[3] - m[0];
    planes[RIGHT].normal.y = m[7] - m[4];
    planes[RIGHT].normal.z = m[11] - m[8];
    planes[RIGHT].distance = m[15] - m[12];

    // Нижняя плоскость
    planes[BOTTOM].normal.x = m[3] + m[1];
    planes[BOTTOM].normal.y = m[7] + m[5];
    planes[BOTTOM].normal.z = m[11] + m[9];
    planes[BOTTOM].distance = m[15] + m[13];

    // Верхняя плоскость
    planes[TOP].normal.x = m[3] - m[1];
    planes[TOP].normal.y = m[7] - m[5];
    planes[TOP].normal.z = m[11] - m[9];
    planes[TOP].distance = m[15] - m[13];

    // Ближняя плоскость
    planes[NEAR].normal.x = m[3] + m[2];
    planes[NEAR].normal.y = m[7] + m[6];
    planes[NEAR].normal.z = m[11] + m[10];
    planes[NEAR].distance = m[15] + m[14];

    // Дальняя плоскость
    planes[FAR].normal.x = m[3] - m[2];
    planes[FAR].normal.y = m[7] - m[6];
    planes[FAR].normal.z = m[11] - m[10];
    planes[FAR].distance = m[15] - m[14];

    // Нормализуем все плоскости
    for (auto& plane : planes) {
        float length = glm::length(plane.normal);
        plane.normal /= length;
        plane.distance /= length;
    }
}

bool Frustum::isBoxVisible(const glm::vec3& minPoint, const glm::vec3& maxPoint) const {
    for (const auto& plane : planes) {
        // Получаем положительную вершину (p-vertex)
        glm::vec3 positiveVertex = minPoint;
        if (plane.normal.x >= 0) positiveVertex.x = maxPoint.x;
        if (plane.normal.y >= 0) positiveVertex.y = maxPoint.y;
        if (plane.normal.z >= 0) positiveVertex.z = maxPoint.z;

        // Если положительная вершина находится за плоскостью, весь бокс невидим
        if (plane.distanceToPoint(positiveVertex) < 0) {
            return false;
        }
    }
    return true;
}

bool Frustum::isSphereVisible(const glm::vec3& center, float radius) const {
    for (const auto& plane : planes) {
        float distance = plane.distanceToPoint(center);
        if (distance < -radius) {
            return false;
        }
    }
    return true;
}

bool Frustum::isPointVisible(const glm::vec3& point) const {
    for (const auto& plane : planes) {
        if (plane.distanceToPoint(point) < 0) {
            return false;
        }
    }
    return true;
}