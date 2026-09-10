#define GLM_ENABLE_EXPERIMENTAL
#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

namespace Lindo {
    namespace Math {
        /**
         * @brief Структура трансформации объекта (позиция, поворот, масштаб).
         */
        struct Transform {
            glm::vec3 position{ 0.0f }; ///< Позиция объекта в пространстве.
            glm::vec3 rotation{ 0.0f }; ///< Поворот в градусах вокруг осей X, Y, Z (углы Эйлера).
            glm::vec3 scale{ 1.0f };    ///< Масштаб объекта по осям.

            /**
             * @brief Рассчитывает локальную матрицу трансформации на основе углов Эйлера.
             * @return Итоговая матрица переноса, поворота и масштаба (glm::mat4).
             */
            glm::mat4 getLocalMatrix() const {
                glm::mat4 model(1.0f);
                model = glm::translate(model, position);
                model = glm::rotate(model, glm::radians(rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
                model = glm::rotate(model, glm::radians(rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
                model = glm::rotate(model, glm::radians(rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
                model = glm::scale(model, scale);
                return model;
            }

            glm::mat4 getMatrix() const { return getLocalMatrix(); }

            glm::vec3 getLocalPosition() const { return position; }
            glm::vec3 getLocalEulerAngles() const { return rotation; }
            glm::vec3 getLocalScale() const { return scale; }

            void setLocalPosition(const glm::vec3& value) { position = value; }
            void setLocalEulerAngles(const glm::vec3& value) { rotation = value; }
            void setLocalScale(const glm::vec3& value) { scale = value; }

            /**
             * @brief Рассчитывает локальную матрицу трансформации через кватернионы для избежания Gimbal Lock.
             * @return Точная матрица модели (glm::mat4).
             */
            glm::mat4 getMatrixQuat() const {
                glm::mat4 model(1.0f);
                model = glm::translate(model, position);

                glm::quat quat = glm::quat(glm::vec3(
                    glm::radians(rotation.x),
                    glm::radians(rotation.y),
                    glm::radians(rotation.z)
                ));
                model = model * glm::mat4_cast(quat);
                model = glm::scale(model, scale);

                return model;
            }

            /**
             * @brief Возвращает нормализованный вектор направления "вперед" (ось +Z) в мировом пространстве.
             * @return Вектор направления glm::vec3.
             */
            glm::vec3 getForward() const {
                glm::quat q = glm::quat(glm::vec3(
                    glm::radians(rotation.x),
                    glm::radians(rotation.y),
                    glm::radians(rotation.z)
                ));
                return glm::normalize(q * glm::vec3(0.0f, 0.0f, 1.0f));
            }

            /**
             * @brief Возвращает нормализованный вектор направления "вверх" (ось +Y) в мировом пространстве.
             * @return Вектор направления glm::vec3.
             */
            glm::vec3 getUp() const {
                glm::quat q = glm::quat(glm::vec3(
                    glm::radians(rotation.x),
                    glm::radians(rotation.y),
                    glm::radians(rotation.z)
                ));
                return glm::normalize(q * glm::vec3(0.0f, 1.0f, 0.0f));
            }

            /**
             * @brief Возвращает нормализованный вектор направления "вправо" (ось +X) в мировом пространстве.
             * @return Вектор направления glm::vec3.
             */
            glm::vec3 getRight() const {
                glm::quat q = glm::quat(glm::vec3(
                    glm::radians(rotation.x),
                    glm::radians(rotation.y),
                    glm::radians(rotation.z)
                ));
                return glm::normalize(q * glm::vec3(1.0f, 0.0f, 0.0f));
            }

            /**
             * @brief Альтернативное получение вектора "вперед" через матрицу поворота.
             * @return Нормализованный вектор направления glm::vec3.
             */
            glm::vec3 getForwardMatrix() const {
                glm::mat4 rotMat = glm::mat4(1.0f);
                rotMat = glm::rotate(rotMat, glm::radians(rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
                rotMat = glm::rotate(rotMat, glm::radians(rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
                rotMat = glm::rotate(rotMat, glm::radians(rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
                return glm::normalize(glm::vec3(rotMat[2][0], rotMat[2][1], rotMat[2][2]));
            }

            /**
             * @brief Альтернативное получение вектора "вверх" через матрицу поворота.
             * @return Нормализованный вектор направления glm::vec3.
             */
            glm::vec3 getUpMatrix() const {
                glm::mat4 rotMat = glm::mat4(1.0f);
                rotMat = glm::rotate(rotMat, glm::radians(rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
                rotMat = glm::rotate(rotMat, glm::radians(rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
                rotMat = glm::rotate(rotMat, glm::radians(rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
                return glm::normalize(glm::vec3(rotMat[1][0], rotMat[1][1], rotMat[1][2]));
            }

            /**
             * @brief Заглушка для обновления внутренних матриц в подсистемах (например, в физическом движке).
             */
            void updateMatrices() {
            }

            /**
             * @brief Поворачивает объект в сторону целевой точки в пространстве.
             * @param target Точка в пространстве, на которую должен смотреть объект.
             * @param up Направляющий вектор "вверх".
             */
            void lookAt(const glm::vec3& target, const glm::vec3& up = glm::vec3(0.0f, 1.0f, 0.0f)) {
                glm::vec3 direction = glm::normalize(target - position);
                glm::mat4 view = glm::lookAt(glm::vec3(0.0f), direction, up);
                glm::quat q = glm::quat_cast(view);

                glm::vec3 euler = glm::eulerAngles(q);
                rotation.x = glm::degrees(euler.x);
                rotation.y = glm::degrees(euler.y);
                rotation.z = glm::degrees(euler.z);
            }
        };
    }
}