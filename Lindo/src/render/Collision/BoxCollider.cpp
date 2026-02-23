
#include "core/OGL.h"
#include "BoxCollider.h"
#include <render/collision/SphereCollider.h>
#include <render/collision/CapsuleCollider.h>

void BoxCollider::generateDebugMesh() const {
    // Очистка старых данных
    debugVertices.clear();
    debugIndices.clear();

    glm::vec3 halfSize = size * 0.5f; // Преобразуем size в extents

    // Вершины куба (8 вершин)
    std::vector<glm::vec3> positions = {
        {-halfSize.x, -halfSize.y, -halfSize.z},
        { halfSize.x, -halfSize.y, -halfSize.z},
        { halfSize.x,  halfSize.y, -halfSize.z},
        {-halfSize.x,  halfSize.y, -halfSize.z},
        {-halfSize.x, -halfSize.y,  halfSize.z},
        { halfSize.x, -halfSize.y,  halfSize.z},
        { halfSize.x,  halfSize.y,  halfSize.z},
        {-halfSize.x,  halfSize.y,  halfSize.z}
    };

    // Индексы (12 треугольников, 2 на каждую грань)
    std::vector<unsigned int> cubeIndices = {
        0, 1, 2, 2, 3, 0, // задняя
        4, 5, 6, 6, 7, 4, // передняя
        0, 4, 7, 7, 3, 0, // левая
        1, 5, 6, 6, 2, 1, // правая
        3, 2, 6, 6, 7, 3, // верхняя
        0, 1, 5, 5, 4, 0  // нижняя
    };

    // Создание массива вершин с нормалями
    for (const auto& pos : positions) {
        // Позиция
        debugVertices.push_back(pos.x);
        debugVertices.push_back(pos.y);
        debugVertices.push_back(pos.z);

        // Нормаль (вычисляется позже на основе индексов)
        debugVertices.push_back(0.0f);
        debugVertices.push_back(0.0f);
        debugVertices.push_back(1.0f);
    }

    debugIndices = cubeIndices;

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

bool BoxCollider::checkRayCollision(const glm::vec3& origin,
    const glm::vec3& direction,
    float* distance,
    glm::vec3* normal) const {
    glm::vec3 center = getCenter();
    glm::vec3 halfSize = size * 0.5f; // Преобразуем size в extents
    glm::vec3 minBounds = center - halfSize;
    glm::vec3 maxBounds = center + halfSize;

    float tmin = (minBounds.x - origin.x) / direction.x;
    float tmax = (maxBounds.x - origin.x) / direction.x;

    if (tmin > tmax) std::swap(tmin, tmax);

    float tymin = (minBounds.y - origin.y) / direction.y;
    float tymax = (maxBounds.y - origin.y) / direction.y;

    if (tymin > tymax) std::swap(tymin, tymax);

    if ((tmin > tymax) || (tymin > tmax))
        return false;

    if (tymin > tmin) tmin = tymin;
    if (tymax < tmax) tmax = tymax;

    float tzmin = (minBounds.z - origin.z) / direction.z;
    float tzmax = (maxBounds.z - origin.z) / direction.z;

    if (tzmin > tzmax) std::swap(tzmin, tzmax);

    if ((tmin > tzmax) || (tzmin > tmax))
        return false;

    if (tzmin > tmin) tmin = tzmin;
    if (tzmax < tmax) tmax = tzmax;

    if (tmax < 0) return false;

    float t = (tmin < 0) ? tmax : tmin;

    if (t >= 0) {
        if (distance) *distance = t;
        if (normal) {
            glm::vec3 hitPoint = origin + direction * t;
            glm::vec3 localHit = hitPoint - center;

            // Определяем, какая грань была пересечена
            glm::vec3 absLocal = glm::abs(localHit);
            if (absLocal.x > absLocal.y && absLocal.x > absLocal.z) {
                *normal = glm::vec3((localHit.x > 0) ? 1.0f : -1.0f, 0.0f, 0.0f);
            }
            else if (absLocal.y > absLocal.z) {
                *normal = glm::vec3(0.0f, (localHit.y > 0) ? 1.0f : -1.0f, 0.0f);
            }
            else {
                *normal = glm::vec3(0.0f, 0.0f, (localHit.z > 0) ? 1.0f : -1.0f);
            }
        }
        return true;
    }

    return false;
}

// В файле BoxCollider.h или .cpp
void BoxCollider::drawDebug(Shader& shader) const {
    if (!isVisible) return;

    glm::mat4 model = transform.getMatrix();
    glm::vec3 halfSize = size * 0.5f;  // если size — полный размер
    model = glm::scale(model, halfSize * 2.0f);  // масштабируем до размеров коллайдера

    shader.setMat4("model", model);
    shader.setVec3("color", debugColor);

    static unsigned int wireCubeVAO = 0;
    static unsigned int wireCubeVBO = 0;
    if (wireCubeVAO == 0) {
        float vertices[] = {
            -0.5f, -0.5f, -0.5f,  0.5f, -0.5f, -0.5f,
             0.5f, -0.5f, -0.5f,  0.5f,  0.5f, -0.5f,
             0.5f,  0.5f, -0.5f, -0.5f,  0.5f, -0.5f,
            -0.5f,  0.5f, -0.5f, -0.5f, -0.5f, -0.5f,

            -0.5f, -0.5f,  0.5f,  0.5f, -0.5f,  0.5f,
             0.5f, -0.5f,  0.5f,  0.5f,  0.5f,  0.5f,
             0.5f,  0.5f,  0.5f, -0.5f,  0.5f,  0.5f,
            -0.5f,  0.5f,  0.5f, -0.5f, -0.5f,  0.5f,

            -0.5f, -0.5f, -0.5f, -0.5f, -0.5f,  0.5f,
             0.5f, -0.5f, -0.5f,  0.5f, -0.5f,  0.5f,
             0.5f,  0.5f, -0.5f,  0.5f,  0.5f,  0.5f,
            -0.5f,  0.5f, -0.5f, -0.5f,  0.5f,  0.5f
        };
        glGenVertexArrays(1, &wireCubeVAO);
        glGenBuffers(1, &wireCubeVBO);
        glBindVertexArray(wireCubeVAO);
        glBindBuffer(GL_ARRAY_BUFFER, wireCubeVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
    }

    glBindVertexArray(wireCubeVAO);
    glDrawArrays(GL_LINES, 0, 24);  // 12 рёбер × 2 вершины
}

bool BoxCollider::checkCollision(const Collider* other, CollisionInfo* info) const {
    if (!other) return false;
    if (other->getType() == ColliderType::CAPSULE) {
        const CapsuleCollider* capsule = static_cast<const CapsuleCollider*>(other);
        return capsule->checkCollisionWithBox(this, info);
    }
    // остальные типы пока заглушки
    return false;
}
