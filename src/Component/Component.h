#pragma once

#include <glm/glm.hpp>
#include "Graphics/core/Shader.h"

namespace Lindo {
    namespace World {
        class GameObject;

        class Component {
        public:
            GameObject* owner = nullptr;

            Component() = default;
            virtual ~Component() = default;

            // Основные методы жизненного цикла
            virtual void OnStart() {}
            virtual void OnUpdate(float deltaTime) {}
            virtual void OnDraw(Graphics::Shader& shader) {}
            virtual void OnDestroy() {}

            // Опциональные методы
            virtual void OnDrawGizmos() {}

            // Звуковые события (оставляем, но можно вынести в отдельный компонент)
            virtual void OnPlaySound() {}
            virtual void OnStopSound() {}
            virtual void OnPauseSound() {}

            // Утилиты
            glm::vec3 getPosition() const;
            void setPosition(const glm::vec3& pos);
        };
    }
}