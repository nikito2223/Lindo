#include <Component/Component.h>
#include "Component/GameObject/GameObject.h"

glm::vec3 Component::getPosition() const {
    return owner ? owner->transform.position : glm::vec3(0.0f);
}

void Component::setPosition(const glm::vec3& pos) {
    if (owner) owner->transform.position = pos;
}