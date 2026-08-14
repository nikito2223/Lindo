#pragma once

#include <glm/glm.hpp>
#include <memory>
#include <string>

namespace Lindo {
    namespace Components {
        namespace Physics {
            class Collider; // Объявляем, что такой класс будет в этом пространстве
            class RigidBody;   // ← ОПЕРЕЖАЮЩЕЕ ОБЪЯВЛЕНИЕ
        }
    }
}

struct RaycastHit {
    glm::vec3 point;
    glm::vec3 normal;
    float distance = 0.0f;
    Lindo::Components::Physics::RigidBody* body;
    Lindo::Components::Physics::Collider* collider = nullptr;
    std::string tag;
};