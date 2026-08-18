#pragma once

#include <glm/glm.hpp>
#include "Graphics/core/Shader.h"

namespace Lindo {
    namespace World {
        class GameObject;

        /// <summary>
        /// Базовый класс для всех компонентов игрового движка.
        /// Управляет жизненным циклом логики и связывает поведение с сущностью GameObject.
        /// </summary>
        class Component {
        public:
            /// <summary>
            /// Указатель на игровой объект (GameObject), которому принадлежит данный компонент.
            /// </summary>
            GameObject* gameObject = nullptr;

            Component() = default;
            virtual ~Component() = default;

            // --- Жизненный цикл компонента ---

            /// <summary> Вызывается один раз при инициализации или добавлении компонента на объект. </summary>
            virtual void OnStart() {}

            /// <summary> Вызывается каждый кадр для обновления логики компонента. </summary>
            /// <param name="deltaTime">Время, прошедшее с прошлого кадра (в секундах).</param>
            virtual void OnUpdate() {}

            /// <summary> Вызывается во время прохода рендеринга. </summary>
            /// <param name="shader">Ссылка на активный шейдер для отрисовки.</param>
            virtual void OnDraw(Graphics::Shader& shader) {}

            /// <summary> Вызывается перед удалением компонента или уничтожением родительского объекта. </summary>
            virtual void OnDestroy() {}

            /// <summary> Отрисовка отладочной графики (сетки, габариты, векторы) в редакторе и при отладке. </summary>
            virtual void OnDrawGizmos() {}

            // --- Утилиты трансформации ---

            /// <summary> Возвращает мировую позицию объекта, к которому прикреплен компонент. </summary>
            glm::vec3 getPosition() const;

            /// <summary> Задает новую мировую позицию объекта. </summary>
            /// <param name="pos">Новые координаты в мировом пространстве.</param>
            void setPosition(const glm::vec3& pos);
        };
    }
}