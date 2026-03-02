#pragma once

#include <glm/glm.hpp>
#include <memory>
#include <string>

class RigidBody;   // ← ОПЕРЕЖАЮЩЕЕ ОБЪЯВЛЕНИЕ

struct RaycastHit {
    glm::vec3 point;
    glm::vec3 normal;
    float distance = 0.0f;
    std::shared_ptr<RigidBody> body;
    std::string tag;
};