#include "core/OGL.h"
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <cmath>
#include <limits>
#include <Physics/Collider/CapsuleCollider.h>
#include <Physics/Collider/BoxCollider.h>
#include <Physics/Collider/SphereCollider.h>

static float distanceSegmentAABB(const glm::vec3& a, const glm::vec3& b,
    const glm::vec3& min, const glm::vec3& max,
    glm::vec3& closestOnSegment) {
    glm::vec3 dir = b - a;
    float tMin = 0.0f, tMax = 1.0f;

    for (int i = 0; i < 3; ++i) {
        if (std::abs(dir[i]) < 1e-6f) {
            if (a[i] < min[i] || a[i] > max[i]) {
                tMin = 1.0f; tMax = 0.0f; // нет пересечения
                break;
            }
        }
        else {
            float t1 = (min[i] - a[i]) / dir[i];
            float t2 = (max[i] - a[i]) / dir[i];
            if (t1 > t2) std::swap(t1, t2);
            tMin = std::max(tMin, t1);
            tMax = std::min(tMax, t2);
            if (tMin > tMax) break;
        }
    }

    if (tMin <= tMax) {
        closestOnSegment = a + ((tMin + tMax) * 0.5f) * dir;
        return 0.0f; // отрезок пересекает AABB
    }

    // Ищем минимальное расстояние до концов отрезка и проекций на грани
    float bestDistSq = std::numeric_limits<float>::max();
    glm::vec3 bestPoint = a;

    auto checkPoint = [&](const glm::vec3& p) {
        glm::vec3 clamped = glm::clamp(p, min, max);
        glm::vec3 diff = p - clamped;
        float distSq = glm::dot(diff, diff);
        if (distSq < bestDistSq) {
            bestDistSq = distSq;
            bestPoint = p;
        }
    };

    checkPoint(a);
    checkPoint(b);

    for (int i = 0; i < 3; ++i) {
        for (float bound : {min[i], max[i]}) {
            if (std::abs(dir[i]) > 1e-6f) {
                float t = (bound - a[i]) / dir[i];
                if (t >= 0.0f && t <= 1.0f) {
                    glm::vec3 p = a + t * dir;
                    checkPoint(p);
                }
            }
        }
    }

    closestOnSegment = bestPoint;
    return std::sqrt(bestDistSq);
}

void CapsuleCollider::generateDebugMesh() const {
    debugVertices.clear();
    debugIndices.clear();

    const int sectors = 16;          // количество сегментов вокруг оси
    const int stacks = 8;            // количество слоёв на полусферу (чётное)
    const float pi = glm::pi<float>();

    float halfHeight = (height - 2.0f * radius) * 0.5f;

    // Верхняя полусфера
    for (int i = 0; i <= stacks / 2; ++i) {
        float stackAngle = pi / 2.0f - i * pi / stacks; // от 90° до 0°
        float xy = radius * cosf(stackAngle);
        float y = radius * sinf(stackAngle) + halfHeight;

        for (int j = 0; j <= sectors; ++j) {
            float sectorAngle = j * 2.0f * pi / sectors;
            float x = xy * cosf(sectorAngle);
            float z = xy * sinf(sectorAngle);

            // Нормаль направлена от центра сферы наружу
            glm::vec3 normal = glm::normalize(glm::vec3(x, y - halfHeight, z));

            debugVertices.push_back(x);
            debugVertices.push_back(y);
            debugVertices.push_back(z);
            debugVertices.push_back(normal.x);
            debugVertices.push_back(normal.y);
            debugVertices.push_back(normal.z);
        }
    }

    // Нижняя полусфера
    for (int i = stacks / 2; i <= stacks; ++i) {
        float stackAngle = pi / 2.0f - i * pi / stacks; // от 0° до -90°
        float xy = radius * cosf(stackAngle);
        float y = -radius * sinf(stackAngle) - halfHeight;

        for (int j = 0; j <= sectors; ++j) {
            float sectorAngle = j * 2.0f * pi / sectors;
            float x = xy * cosf(sectorAngle);
            float z = xy * sinf(sectorAngle);

            // Нормаль направлена от центра сферы наружу
            glm::vec3 normal = glm::normalize(glm::vec3(x, y + halfHeight, z));

            debugVertices.push_back(x);
            debugVertices.push_back(y);
            debugVertices.push_back(z);
            debugVertices.push_back(normal.x);
            debugVertices.push_back(normal.y);
            debugVertices.push_back(normal.z);
        }
    }

    // Генерация индексов (одинаково для всей капсулы)
    int totalStacks = stacks + 1;              // количество слоёв вершин
    int totalVerticesPerStack = sectors + 1;

    for (int i = 0; i < stacks; ++i) {
        for (int j = 0; j < sectors; ++j) {
            int first = i * totalVerticesPerStack + j;
            int second = first + totalVerticesPerStack;

            // Два треугольника на каждый четырёхугольник
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

    // Позиции (атрибут 0)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Нормали (атрибут 1)
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float),
        (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
}



glm::vec3 CapsuleCollider::getTopSphereCenter() const {
    float halfCylinder = (height - 2.0f * radius) * 0.5f;
    return getPosition() + glm::vec3(0.0f, halfCylinder, 0.0f);
}

glm::vec3 CapsuleCollider::getBottomSphereCenter() const {
    float halfCylinder = (height - 2.0f * radius) * 0.5f;
    return getPosition() - glm::vec3(0.0f, halfCylinder, 0.0f);
}

void CapsuleCollider::drawDebug(Shader& shader) const {
    if (!isVisible) return;
    if (VAO == 0) return;

    glm::mat4 model = owner->transform.getMatrix();
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

bool CapsuleCollider::checkCollision(const Collider* other, CollisionInfo* info) const {
    if (!other) return false;

    switch (other->getType()) {
    case ColliderType::BOX: {
        const BoxCollider* box = static_cast<const BoxCollider*>(other);
        return checkCollisionWithBox(box, info);
    }
    case ColliderType::SPHERE:
        // TODO
        return false;
    case ColliderType::CAPSULE:
        // TODO
        return false;
    default:
        return false;
    }
}

bool CapsuleCollider::checkCollisionWithBox(const BoxCollider* box, CollisionInfo* info) const {
    glm::vec3 top = getTopSphereCenter();
    glm::vec3 bottom = getBottomSphereCenter();
    float r = radius;

    glm::vec3 boxCenter = box->getCenter();
    glm::vec3 halfSize = box->getHalfSize(); // предполагается, что метод есть
    glm::vec3 boxMin = boxCenter - halfSize;
    glm::vec3 boxMax = boxCenter + halfSize;

    glm::vec3 closestOnSegment;
    float dist = distanceSegmentAABB(bottom, top, boxMin, boxMax, closestOnSegment);
    glm::vec3 closestOnBox = glm::clamp(closestOnSegment, boxMin, boxMax);

    if (dist < r) {
        if (info) {
            glm::vec3 diff = closestOnSegment - closestOnBox;
            if (dist > 0) {
                info->normal = diff / dist;
                info->penetration = r - dist;
            }
            else {
                // Центр капсулы внутри бокса – выбираем ближайшую грань
                glm::vec3 local = closestOnSegment - boxCenter;
                glm::vec3 absLocal = glm::abs(local);
                glm::vec3 distances = halfSize - absLocal;

                if (distances.x < distances.y && distances.x < distances.z) {
                    info->normal = glm::vec3(local.x > 0 ? 1.0f : -1.0f, 0.0f, 0.0f);
                    info->penetration = r + distances.x;
                }
                else if (distances.y < distances.z) {
                    info->normal = glm::vec3(0.0f, local.y > 0 ? 1.0f : -1.0f, 0.0f);
                    info->penetration = r + distances.y;
                }
                else {
                    info->normal = glm::vec3(0.0f, 0.0f, local.z > 0 ? 1.0f : -1.0f);
                    info->penetration = r + distances.z;
                }
            }
            info->point = closestOnBox;
        }
        return true;
    }
    return false;
}


bool CapsuleCollider::checkRayCollision(const glm::vec3& origin,
    const glm::vec3& direction,
    float* distance,
    glm::vec3* normal) const {
    // TODO: Реализовать проверку пересечения луча с капсулой
    return false;
}
