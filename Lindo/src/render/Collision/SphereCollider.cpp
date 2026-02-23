
#pragma once
#include "core/OGL.h"
#include "render/Collision/CapsuleCollider.h"
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <cmath>
#include "render/Collision/BoxCollider.h"
#include "render/Collision/SphereCollider.h"

void SphereCollider::generateDebugMesh() const {
    // Очистка старых данных
    debugVertices.clear();
    debugIndices.clear();

    // Параметры для генерации
    const int sectors = 16;
    const int stacks = 16;
    const float pi = glm::pi<float>();

    // Генерация вершин
    for (int i = 0; i <= stacks; ++i) {
        float stackAngle = pi / 2.0f - i * pi / stacks;
        float xy = radius * cosf(stackAngle);
        float z = radius * sinf(stackAngle);

        for (int j = 0; j <= sectors; ++j) {
            float sectorAngle = j * 2.0f * pi / sectors;

            float x = xy * cosf(sectorAngle);
            float y = xy * sinf(sectorAngle);

            // Нормаль
            glm::vec3 normal = glm::normalize(glm::vec3(x, y, z));

            // Вершина
            debugVertices.push_back(x);
            debugVertices.push_back(y);
            debugVertices.push_back(z);
            debugVertices.push_back(normal.x);
            debugVertices.push_back(normal.y);
            debugVertices.push_back(normal.z);
        }
    }

    // Генерация индексов
    for (int i = 0; i < stacks; ++i) {
        for (int j = 0; j < sectors; ++j) {
            int first = i * (sectors + 1) + j;
            int second = first + sectors + 1;

            debugIndices.push_back(first);
            debugIndices.push_back(second);
            debugIndices.push_back(first + 1);

            debugIndices.push_back(second);
            debugIndices.push_back(second + 1);
            debugIndices.push_back(first + 1);
        }
    }

    // Создание OpenGL объектов
    if (VAO == 0) {
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glGenBuffers(1, &EBO);
    }

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, debugVertices.size() * sizeof(float),
        debugVertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, debugIndices.size() * sizeof(unsigned int),
        debugIndices.data(), GL_STATIC_DRAW);

    // Позиции
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Нормали
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float),
        (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
}

bool SphereCollider::checkCollision(const Collider* other, CollisionInfo* info) const {
    if (!other) return false;

    switch (other->getType()) {
    case ColliderType::SPHERE: {
        const SphereCollider* sphere = static_cast<const SphereCollider*>(other);
        glm::vec3 diff = getCenter() - sphere->getCenter();
        float distanceSq = glm::dot(diff, diff);
        float combinedRadius = radius + sphere->getRadius();

        if (distanceSq <= combinedRadius * combinedRadius) {
            if (info) {
                float distance = sqrt(distanceSq);
                info->normal = (distance > 0) ? diff / distance : glm::vec3(0, 1, 0);
                info->penetration = combinedRadius - distance;
                info->point = getCenter() - info->normal * radius;
            }
            return true;
        }
        return false;
    }
    case ColliderType::BOX: {
        const BoxCollider* box = static_cast<const BoxCollider*>(other);
        // TODO: Реализовать проверку столкновения сферы и бокса
        return false;
    }
    case ColliderType::CAPSULE: {
        const CapsuleCollider* capsule = static_cast<const CapsuleCollider*>(other);
        // TODO: Реализовать проверку столкновения сферы и капсулы
        return false;
    }
    default:
        return false;
    }
}

bool SphereCollider::checkRayCollision(const glm::vec3& origin,
    const glm::vec3& direction,
    float* distance,
    glm::vec3* normal) const {
    glm::vec3 center = getCenter();
    glm::vec3 oc = origin - center;

    float a = glm::dot(direction, direction);
    float b = 2.0f * glm::dot(oc, direction);
    float c = glm::dot(oc, oc) - radius * radius;

    float discriminant = b * b - 4.0f * a * c;

    if (discriminant < 0.0f) {
        return false;
    }

    float sqrtDiscriminant = sqrt(discriminant);
    float t1 = (-b - sqrtDiscriminant) / (2.0f * a);
    float t2 = (-b + sqrtDiscriminant) / (2.0f * a);

    float t = (t1 > 0.0f) ? t1 : ((t2 > 0.0f) ? t2 : -1.0f);

    if (t > 0.0f) {
        if (distance) *distance = t;
        if (normal) {
            *normal = glm::normalize((origin + direction * t) - center);
        }
        return true;
    }

    return false;
}

void SphereCollider::drawDebug(Shader& shader) const {
    if (!isVisible) return;
    if (VAO == 0) return;  // предполагается, что generateDebugMesh() уже вызван

    glm::mat4 model = transform.getMatrix();
    shader.setMat4("model", model);
    shader.setVec3("color", debugColor);

    GLint polygonMode[2];
    glGetIntegerv(GL_POLYGON_MODE, polygonMode);
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, debugIndices.size(), GL_UNSIGNED_INT, 0);

    glPolygonMode(GL_FRONT_AND_BACK, polygonMode[0]);
    glBindVertexArray(0);
}