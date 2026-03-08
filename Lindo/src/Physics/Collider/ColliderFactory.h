// ColliderFactory.h
#pragma once

#include "BoxCollider.h"
#include "SphereCollider.h"
#include "CapsuleCollider.h"
#include <functional>
#include <Component/GameObject/GameObject.h>

class ColliderFactory {
public:
    // Создание коллайдера по типу
    static std::shared_ptr<Collider> createCollider(ColliderType type,
        const glm::vec3& size = glm::vec3(1.0f)) {
        switch (type) {
        case ColliderType::BOX:
            return std::make_shared<BoxCollider>(size);
        case ColliderType::SPHERE:
            return std::make_shared<SphereCollider>(size.x);
        case ColliderType::CAPSULE:
            return std::make_shared<CapsuleCollider>(size.x, size.y);
        default:
            return nullptr;
        }
    }

    // Создание коллайдера для объекта
    static std::shared_ptr<Collider> createColliderForObject(GameObject* object,
        ColliderType type = ColliderType::BOX) {
        if (!object) return nullptr;

        std::shared_ptr<Collider> col;

        switch (type) {
        case ColliderType::BOX:
            col = std::make_shared<BoxCollider>(object->transform.scale);
            break;
        case ColliderType::SPHERE:
            float radius = std::max({ object->transform.scale.x, object->transform.scale.y, object->transform.scale.z }) * 0.5f;
            col = std::make_shared<SphereCollider>(radius);
            break;
        case ColliderType::CAPSULE:
            col = std::make_shared<CapsuleCollider>(object->transform.scale.x * 0.5f, object->transform.scale.y);
            break;
        default:
            return nullptr;
        }

        col->owner = object;

        return col;
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