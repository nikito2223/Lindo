#include <Component/Component.h>
#include "Component/GameObject/GameObject.h"

namespace Lindo {
    namespace World {

        glm::vec3 Component::getPosition() const {
            return gameObject ? gameObject->transform.position : glm::vec3(0.0f);
        }

        void Component::setPosition(const glm::vec3& pos) {
            if (gameObject) gameObject->transform.position = pos;
        }
    }
}