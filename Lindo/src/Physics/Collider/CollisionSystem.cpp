#include "CollisionSystem.h"
#include <algorithm>
#include <iostream>


// =============== ВСПОМОГАТЕЛЬНЫЕ ФУНКЦИИ ===============

// Проверка Box-Box
static bool checkBoxBox(const BoxCollider* a, const BoxCollider* b, CollisionInfo* info) {
    glm::vec3 centerA = a->getTransform().position;
    glm::vec3 centerB = b->getTransform().position;
    glm::vec3 extentsA = a->getExtents();
    glm::vec3 extentsB = b->getExtents();

    // Простая AABB проверка
    glm::vec3 delta = centerA - centerB;
    glm::vec3 totalExtents = extentsA + extentsB;

    if (fabs(delta.x) > totalExtents.x ||
        fabs(delta.y) > totalExtents.y ||
        fabs(delta.z) > totalExtents.z) {
        return false;
    }

    if (info) {
        // Вычисляем нормаль и глубину
        glm::vec3 overlap;
        overlap.x = totalExtents.x - fabs(delta.x);
        overlap.y = totalExtents.y - fabs(delta.y);
        overlap.z = totalExtents.z - fabs(delta.z);

        // Находим минимальное перекрытие
        float minOverlap = std::min({ overlap.x, overlap.y, overlap.z });

        info->hasCollision = true;
        info->depth = minOverlap;

        // Определяем нормаль
        if (minOverlap == overlap.x) {
            info->normal = glm::vec3(delta.x > 0 ? 1.0f : -1.0f, 0.0f, 0.0f);
        }
        else if (minOverlap == overlap.y) {
            info->normal = glm::vec3(0.0f, delta.y > 0 ? 1.0f : -1.0f, 0.0f);
        }
        else {
            info->normal = glm::vec3(0.0f, 0.0f, delta.z > 0 ? 1.0f : -1.0f);
        }

        info->contactPoint = centerB + info->normal * (extentsB - glm::vec3(info->depth * 0.5f));
    }

    return true;
}

// Проверка Sphere-Sphere
static bool checkSphereSphere(const SphereCollider* a, const SphereCollider* b, CollisionInfo* info) {
    glm::vec3 centerA = a->getTransform().position;
    glm::vec3 centerB = b->getTransform().position;
    float radiusA = a->getRadius();
    float radiusB = b->getRadius();

    glm::vec3 delta = centerA - centerB;
    float distanceSq = glm::dot(delta, delta);
    float radiusSum = radiusA + radiusB;

    if (distanceSq > radiusSum * radiusSum) {
        return false;
    }

    if (info) {
        float distance = sqrt(distanceSq);
        info->hasCollision = true;
        info->depth = radiusSum - distance;
        info->normal = glm::normalize(delta);
        info->contactPoint = centerB + info->normal * radiusB;
    }

    return true;
}

// Проверка Sphere-Box
static bool checkSphereBox(const SphereCollider* sphere, const BoxCollider* box, CollisionInfo* info) {
    glm::vec3 sphereCenter = sphere->getTransform().position;
    glm::vec3 boxCenter = box->getTransform().position;
    glm::vec3 boxExtents = box->getExtents();
    float sphereRadius = sphere->getRadius();

    // Находим ближайшую точку на AABB к сфере
    glm::vec3 closestPoint;
    closestPoint.x = std::max(boxCenter.x - boxExtents.x,
        std::min(sphereCenter.x, boxCenter.x + boxExtents.x));
    closestPoint.y = std::max(boxCenter.y - boxExtents.y,
        std::min(sphereCenter.y, boxCenter.y + boxExtents.y));
    closestPoint.z = std::max(boxCenter.z - boxExtents.z,
        std::min(sphereCenter.z, boxCenter.z + boxExtents.z));

    // Проверяем расстояние
    glm::vec3 delta = sphereCenter - closestPoint;
    float distanceSq = glm::dot(delta, delta);

    if (distanceSq > sphereRadius * sphereRadius) {
        return false;
    }

    if (info) {
        float distance = sqrt(distanceSq);
        info->hasCollision = true;
        info->depth = sphereRadius - distance;

        if (distance > 0.0001f) {
            info->normal = glm::normalize(delta);
        }
        else {
            // Сфера внутри бокса
            info->normal = glm::vec3(0.0f, 1.0f, 0.0f);
        }

        info->contactPoint = closestPoint;
    }

    return true;
}

// =============== РЕАЛИЗАЦИЯ COLLISIONSYSTEM ===============

void CollisionSystem::addCollider(std::shared_ptr<Collider> collider, const std::string& tag) {
    ColliderEntry entry;
    entry.collider = collider;
    entry.tag = tag;

    // Получаем слой из маппинга тегов
    auto it = tagToLayer.find(tag);
    if (it != tagToLayer.end()) {
        entry.layer = it->second;
    }
    else {
        entry.layer = 0; // Слой по умолчанию
    }

    colliders.push_back(entry);
}

void CollisionSystem::removeCollider(std::shared_ptr<Collider> collider) {
    // Находим коллайдер по указателю
    auto it = std::remove_if(colliders.begin(), colliders.end(),
        [collider](const ColliderEntry& entry) {
            return entry.collider == collider;
        });

    if (it != colliders.end()) {
        colliders.erase(it, colliders.end());
    }
}

void CollisionSystem::clearColliders() {
    colliders.clear();
}

static bool checkCapsuleBox(const CapsuleCollider* capsule, const BoxCollider* box, CollisionInfo* info) {
    // Получаем центры полусфер капсулы в мировых координатах
    glm::vec3 topSphere = capsule->getTopSphereCenter();
    glm::vec3 bottomSphere = capsule->getBottomSphereCenter();
    float radius = capsule->getRadius();

    // Центр и половина размера бокса
    glm::vec3 boxCenter = box->getCenter();
    glm::vec3 boxHalfSize = box->getExtents(); // уже половина

    // Функция поиска ближайшей точки на отрезке [A, B] к точке P
    auto closestPointOnSegment = [](const glm::vec3& A, const glm::vec3& B, const glm::vec3& P) -> glm::vec3 {
        glm::vec3 AB = B - A;
        float t = glm::dot(P - A, AB) / glm::dot(AB, AB);
        t = glm::clamp(t, 0.0f, 1.0f);
        return A + t * AB;
    };

    // Сначала найдём ближайшую точку на отрезке капсулы к боксу
    // Для этого можно использовать алгоритм GJK или просто проверить 8 вершин и центр
    // Но проще: найдём ближайшую точку на отрезке к центру бокса, затем проверим расстояние до AABB.
    glm::vec3 closestOnLine = closestPointOnSegment(bottomSphere, topSphere, boxCenter);

    // Теперь найдём ближайшую точку на AABB к этой точке
    glm::vec3 closestOnBox;
    closestOnBox.x = glm::clamp(closestOnLine.x, boxCenter.x - boxHalfSize.x, boxCenter.x + boxHalfSize.x);
    closestOnBox.y = glm::clamp(closestOnLine.y, boxCenter.y - boxHalfSize.y, boxCenter.y + boxHalfSize.y);
    closestOnBox.z = glm::clamp(closestOnLine.z, boxCenter.z - boxHalfSize.z, boxCenter.z + boxHalfSize.z);

    // Вектор от ближайшей точки на боксе до ближайшей точки на отрезке
    glm::vec3 delta = closestOnLine - closestOnBox;
    float distSq = glm::dot(delta, delta);

    if (distSq > radius * radius) return false; // нет пересечения

    // Пересечение есть
    if (info) {
        float dist = sqrt(distSq);
        info->hasCollision = true;
        info->depth = radius - dist;
        if (dist > 1e-6f) {
            info->normal = glm::normalize(delta);
        }
        else {
            // Капсула касается бокса или центр капсулы внутри – выбираем нормаль от ближайшей грани
            glm::vec3 localLine = closestOnLine - boxCenter;
            glm::vec3 absLocal = glm::abs(localLine);
            if (absLocal.x > absLocal.y && absLocal.x > absLocal.z) {
                info->normal = glm::vec3((localLine.x > 0) ? 1.0f : -1.0f, 0.0f, 0.0f);
            }
            else if (absLocal.y > absLocal.z) {
                info->normal = glm::vec3(0.0f, (localLine.y > 0) ? 1.0f : -1.0f, 0.0f);
            }
            else {
                info->normal = glm::vec3(0.0f, 0.0f, (localLine.z > 0) ? 1.0f : -1.0f);
            }
        }
        // Если контакт почти вертикальный, делаем нормаль строго вертикальной
        if (fabs(info->normal.y) > 0.7f) {
            info->normal = glm::vec3(0.0f, glm::sign(info->normal.y), 0.0f);
        }
        info->contactPoint = closestOnBox;
    } 

    return true;
}

std::vector<std::shared_ptr<Collider>> CollisionSystem::getCollidersByTag(const std::string& tag) const {
    std::vector<std::shared_ptr<Collider>> result;

    for (const auto& entry : colliders) {
        if (entry.tag == tag) {
            result.push_back(entry.collider);
        }
    }

    return result;
}

void CollisionSystem::update(float deltaTime) {
    // Проверяем все пары коллайдеров
    for (size_t i = 0; i < colliders.size(); ++i) {
        for (size_t j = i + 1; j < colliders.size(); ++j) {
            // Проверяем слои коллизий
            int layerA = colliders[i].layer;
            int layerB = colliders[j].layer;

            // Проверяем, могут ли слои сталкиваться
            bool canCollide = true;
            auto layerItA = layerCollisionMatrix.find(layerA);
            if (layerItA != layerCollisionMatrix.end()) {
                auto layerItB = layerItA->second.find(layerB);
                if (layerItB != layerItA->second.end()) {
                    canCollide = layerItB->second;
                }
            }

            if (canCollide) {
                checkPairCollision(i, j);
            }
        }
    }
}

bool CollisionSystem::checkCollision(std::shared_ptr<Collider> a, std::shared_ptr<Collider> b,
    CollisionInfo* info) const {
    if (!a || !b) return false;

    // Проверка слоёв (оставляем как есть)
    int layerA = 0;
    int layerB = 0;
    for (const auto& entry : colliders) {
        if (entry.collider == a) layerA = entry.layer;
        if (entry.collider == b) layerB = entry.layer;
    }
    auto layerItA = layerCollisionMatrix.find(layerA);
    if (layerItA != layerCollisionMatrix.end()) {
        auto layerItB = layerItA->second.find(layerB);
        if (layerItB != layerItA->second.end() && !layerItB->second) {
            return false; // слои не могут сталкиваться
        }
    }

    // Диспетчеризация по типам
    ColliderType typeA = a->getType();
    ColliderType typeB = b->getType();

    if (typeA == ColliderType::BOX && typeB == ColliderType::BOX) {
        return checkBoxBox(static_cast<BoxCollider*>(a.get()),
            static_cast<BoxCollider*>(b.get()), info);
    }
    else if (typeA == ColliderType::CAPSULE && typeB == ColliderType::BOX) {
        return checkCapsuleBox(static_cast<CapsuleCollider*>(a.get()),
            static_cast<BoxCollider*>(b.get()), info);
    }
    else if (typeA == ColliderType::BOX && typeB == ColliderType::CAPSULE) {
        bool result = checkCapsuleBox(static_cast<CapsuleCollider*>(b.get()),
            static_cast<BoxCollider*>(a.get()), info);
        if (result && info) info->normal = -info->normal;
        return result;
    }
    else if (typeA == ColliderType::BOX && typeB == ColliderType::SPHERE) {
        // Для пары (BOX, SPHERE) вызываем checkSphereBox с переставленными аргументами
        bool result = checkSphereBox(static_cast<SphereCollider*>(b.get()),
            static_cast<BoxCollider*>(a.get()), info);
        if (result && info) {
            // Инвертируем нормаль, так как порядок reversed
            info->normal = -info->normal;
        }
        return result;
    }
    // TODO: Добавить обработку CAPSULE и других типов по мере реализации
    else {
        // Если комбинация не обработана, пробуем виртуальный метод (на случай пользовательских реализаций)
        return a->checkCollision(b.get(), info);
    }
}

CollisionSystem::RaycastResult CollisionSystem::raycast(const glm::vec3& origin,
    const glm::vec3& direction,
    float maxDistance,
    const std::string& tagFilter) const {

    RaycastResult result;
    result.distance = maxDistance;

    for (const auto& entry : colliders) {
        // Проверяем фильтр по тегу
        if (!tagFilter.empty() && entry.tag != tagFilter) {
            continue;
        }

        float distance;
        glm::vec3 normal;

        if (entry.collider->checkRayCollision(origin, direction, &distance, &normal)) {
            if (distance < result.distance && distance > 0.0f) {
                result.hit = true;
                result.collider = entry.collider;
                result.distance = distance;
                result.normal = normal;
                result.point = origin + direction * distance;
            }
        }
    }

    return result;
}

void CollisionSystem::registerCollisionCallback(const std::string& tagA,
    const std::string& tagB,
    CollisionCallback callback) {

    // Создаем ключ для пары тегов (упорядоченный)
    std::string key = tagA < tagB ? tagA + "_" + tagB : tagB + "_" + tagA;
    collisionCallbacks[key].push_back(callback);
}

void CollisionSystem::registerTriggerCallback(const std::string& tag,
    CollisionCallback callback) {

    triggerCallbacks[tag].push_back(callback);
}

void CollisionSystem::drawDebug(Shader& shader) const {
    for (const auto& entry : colliders) {
        if (entry.collider->getVisible()) {
            entry.collider->drawDebug(shader);
        }
    }
}

void CollisionSystem::setCollisionLayer(const std::string& tag, int layer) {
    tagToLayer[tag] = layer;

    // Обновляем слой для существующих коллайдеров с этим тегом
    for (auto& entry : colliders) {
        if (entry.tag == tag) {
            entry.layer = layer;
        }
    }
}

void CollisionSystem::setLayerCollision(int layerA, int layerB, bool canCollide) {
    layerCollisionMatrix[layerA][layerB] = canCollide;
    layerCollisionMatrix[layerB][layerA] = canCollide;
}

// =============== ВНУТРЕННИЕ МЕТОДЫ ===============

void CollisionSystem::checkPairCollision(size_t i, size_t j) {
    auto& entryA = colliders[i];
    auto& entryB = colliders[j];

    // Проверяем триггеры
    bool isTrigger = entryA.collider->getTrigger() || entryB.collider->getTrigger();

    CollisionInfo info;
    bool hasCollision = false;

    // Производим проверку столкновения
    if (entryA.collider->getType() == ColliderType::BOX &&
        entryB.collider->getType() == ColliderType::BOX) {
        hasCollision = checkBoxBox(
            static_cast<BoxCollider*>(entryA.collider.get()),
            static_cast<BoxCollider*>(entryB.collider.get()),
            &info
        );
    }
    else if (entryA.collider->getType() == ColliderType::SPHERE &&
        entryB.collider->getType() == ColliderType::SPHERE) {
        hasCollision = checkSphereSphere(
            static_cast<SphereCollider*>(entryA.collider.get()),
            static_cast<SphereCollider*>(entryB.collider.get()),
            &info
        );
    }
    else if (entryA.collider->getType() == ColliderType::SPHERE &&
        entryB.collider->getType() == ColliderType::BOX) {
        hasCollision = checkSphereBox(
            static_cast<SphereCollider*>(entryA.collider.get()),
            static_cast<BoxCollider*>(entryB.collider.get()),
            &info
        );
    }
    else if (entryA.collider->getType() == ColliderType::BOX &&
        entryB.collider->getType() == ColliderType::SPHERE) {
        // Меняем порядок для consistency
        info.normal = -info.normal;
        hasCollision = checkSphereBox(
            static_cast<SphereCollider*>(entryB.collider.get()),
            static_cast<BoxCollider*>(entryA.collider.get()),
            &info
        );
        if (hasCollision && info.hasCollision) {
            info.normal = -info.normal;
        }
    }
    else {
        // Для других типов используем общий метод
        hasCollision = entryA.collider->checkCollision(entryB.collider.get(), &info);
    }

    if (hasCollision && info.hasCollision) {
        bool isEnter = !(entryA.wasColliding && entryB.wasColliding);
        fireCollisionEvent(entryA, entryB, info, isEnter);

        // Обновляем флаги столкновения
        colliders[i].wasColliding = true;
        colliders[j].wasColliding = true;
    }
    else {
        // Если столкновение прекратилось
        if (entryA.wasColliding && entryB.wasColliding) {
            info.hasCollision = false;
            fireCollisionEvent(entryA, entryB, info, false);
        }

        // Сбрасываем флаги
        colliders[i].wasColliding = false;
        colliders[j].wasColliding = false;
    }
}

void CollisionSystem::fireCollisionEvent(const ColliderEntry& a, const ColliderEntry& b,
    const CollisionInfo& info, bool isEnter) {

    // Создаем событие
    CollisionEvent event;
    event.colliderA = a.collider;
    event.colliderB = b.collider;
    event.info = info;
    event.isEnter = isEnter;
    event.timestamp = 0.0f; // TODO: Добавить таймер

    // Вызываем колбэки триггеров
    if (a.collider->getTrigger()) {
        auto it = triggerCallbacks.find(a.tag);
        if (it != triggerCallbacks.end()) {
            for (auto& callback : it->second) {
                callback(event);
            }
        }
    }

    if (b.collider->getTrigger()) {
        auto it = triggerCallbacks.find(b.tag);
        if (it != triggerCallbacks.end()) {
            for (auto& callback : it->second) {
                callback(event);
            }
        }
    }

    // Вызываем колбэки столкновений
    if (!a.collider->getTrigger() && !b.collider->getTrigger()) {
        // Создаем ключ для пары тегов (упорядоченный)
        std::string key = a.tag < b.tag ? a.tag + "_" + b.tag : b.tag + "_" + a.tag;
        auto it = collisionCallbacks.find(key);
        if (it != collisionCallbacks.end()) {
            for (auto& callback : it->second) {
                callback(event);
            }
        }
    }
}