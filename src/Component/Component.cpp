#include <Component/Component.h>
#include "Component/GameObject/GameObject.h"

namespace Lindo {
    namespace World {

        /**
         * @brief Возвращает мировую позицию через трансформ родительского GameObject.
         * @return Позиция объекта или glm::vec3(0.0f), если gameObject равен nullptr.
         */
        glm::vec3 Component::getPosition() const {
            return gameObject ? gameObject->transform.position : glm::vec3(0.0f);
        }

        /**
         * @brief Устанавливает новую позицию в трансформ родительского GameObject.
         * @param pos Новые координаты.
         */
        void Component::setPosition(const glm::vec3& pos) {
            if (gameObject) {
                gameObject->transform.position = pos;
            }
        }
    }
}