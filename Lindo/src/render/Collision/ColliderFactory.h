// ColliderFactory.h
#pragma once

#include "BoxCollider.h"
#include "SphereCollider.h"
#include "CapsuleCollider.h"
#include <functional>
#include <render/objects/Object.h>

class ColliderFactory {
public:
    // Создание коллайдера по типу
    static std::shared_ptr<Collider> createCollider(ColliderType type,
        const Transform& transform = Transform(),
        const glm::vec3& size = glm::vec3(1.0f)) {
        switch (type) {
        case ColliderType::BOX:
            return std::make_shared<BoxCollider>(size, transform);
        case ColliderType::SPHERE:
            return std::make_shared<SphereCollider>(size.x, transform);
        case ColliderType::CAPSULE:
            return std::make_shared<CapsuleCollider>(size.x, size.y, transform);
        default:
            return nullptr;
        }
    }

    // Создание коллайдера для объекта
    static std::shared_ptr<Collider> createColliderForObject(Object* object,
        ColliderType type = ColliderType::BOX) {
        if (!object) return nullptr;

        Transform transform = object->transform;

        // Автоматическое определение размера на основе масштаба
        glm::vec3 size = transform.scale;

        switch (type) {
        case ColliderType::BOX:
            return std::make_shared<BoxCollider>(size, transform);
        case ColliderType::SPHERE:
            // Используем максимальное измерение для радиуса
            float radius = std::max({ size.x, size.y, size.z }) * 0.5f;
            return std::make_shared<SphereCollider>(radius, transform);
        case ColliderType::CAPSULE:
            return std::make_shared<CapsuleCollider>(size.x * 0.5f, size.y, transform);
        default:
            return nullptr;
        }
    }

    // Пресеты для различных объектов
    static std::shared_ptr<BoxCollider> createPlayerCollider() {
        return std::make_shared<BoxCollider>(0.8f, 1.8f, 0.8f);
    }

    static std::shared_ptr<CapsuleCollider> createCharacterCollider() {
        return std::make_shared<CapsuleCollider>(0.4f, 1.6f);
    }

    static std::shared_ptr<SphereCollider> createProjectileCollider(float radius = 0.2f) {
        return std::make_shared<SphereCollider>(radius);
    }

    static std::shared_ptr<BoxCollider> createPlatformCollider(float width = 10.0f,
        float height = 0.1f,
        float depth = 10.0f) {
        return std::make_shared<BoxCollider>(width, height, depth);
    }
};