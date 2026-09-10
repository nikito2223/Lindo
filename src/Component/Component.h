#pragma once

#include <glm/glm.hpp>
#include "Graphics/core/Shader.h"

namespace Lindo {
    namespace World {
        class GameObject;

        /**
         * @brief Базовый класс для всех компонентов игрового движка.
         *
         * Управляет жизненным циклом логики и связывает поведение с сущностью GameObject.
         */
        class Component {
        public:
            GameObject* gameObject = nullptr; ///< Указатель на родительский игровой объект.
            bool enabledGizmos = true;        ///< Флаг активности отрисовки отладочной графики.

            Component() = default;
            virtual ~Component() = default;

            /**
             * @brief Вызывается один раз при инициализации или добавлении компонента на объект.
             */
            virtual void OnStart() {}

            /**
             * @brief Вызывается каждый кадр для обновления логики компонента.
             */
            virtual void OnUpdate() {}

            /**
             * @brief Вызывается во время прохода рендеринга.
             * @param shader Ссылка на активный шейдер для отрисовки.
             */
            virtual void OnDraw(Graphics::Shader& shader) {}

            /**
             * @brief Вызывается перед удалением компонента или уничтожением родительского объекта.
             */
            virtual void OnDestroy() {}

            /**
             * @brief Отрисовка отладочной графики (сетки, хитбоксы, векторы) в редакторе или режиме отладки.
             */
            virtual void OnDrawGizmos() {}

            /**
             * @brief Получает мировую позицию объекта, к которому прикреплен компонент.
             * @return Вектор мировой позиции glm::vec3.
             */
            glm::vec3 getPosition() const;

            /**
             * @brief Задает новую мировую позицию родительского объекта.
             * @param pos Новые координаты в мировом пространстве.
             */
            void setPosition(const glm::vec3& pos);
        };
    }
}