#define GLM_ENABLE_EXPERIMENTAL
#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
namespace Lindo {
    namespace Math {
        struct Transform {
            glm::vec3 position{ 0.0f };
            glm::vec3 rotation{ 0.0f }; // градусы вокруг осей X, Y, Z
            glm::vec3 scale{ 1.0f };

            glm::mat4 getMatrix() const {
                glm::mat4 model(1.0f);
                // 1. Перемещение
                model = glm::translate(model, position);
                // 2. Вращение (X, Y, Z)
                model = glm::rotate(model, glm::radians(rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
                model = glm::rotate(model, glm::radians(rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
                model = glm::rotate(model, glm::radians(rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
                // 3. Масштабирование
                model = glm::scale(model, scale);
                return model;
            }

            // Альтернативный вариант с кватернионами (более точный)
            glm::mat4 getMatrixQuat() const {
                glm::mat4 model(1.0f);

                // Перемещение
                model = glm::translate(model, position);

                // Вращение через кватернион
                glm::quat quat = glm::quat(glm::vec3(
                    glm::radians(rotation.x),
                    glm::radians(rotation.y),
                    glm::radians(rotation.z)
                ));
                model = model * glm::mat4_cast(quat);

                // Масштабирование
                model = glm::scale(model, scale);

                return model;
            }

            glm::vec3 getForward() const {
                // Создаем кватернион из углов Эйлера (в радианах)
                glm::quat q = glm::quat(glm::vec3(
                    glm::radians(rotation.x),
                    glm::radians(rotation.y),
                    glm::radians(rotation.z)
                ));

                // Поворачиваем вектор (0,0,1) кватернионом
                return glm::normalize(q * glm::vec3(0.0f, 0.0f, 1.0f));
            }

            // Получить направление "вверх" (локальная ось Y) в мировых координатах
            glm::vec3 getUp() const {
                // Создаем кватернион из углов Эйлера (в радианах)
                glm::quat q = glm::quat(glm::vec3(
                    glm::radians(rotation.x),
                    glm::radians(rotation.y),
                    glm::radians(rotation.z)
                ));

                // Поворачиваем вектор (0,1,0) кватернионом
                return glm::normalize(q * glm::vec3(0.0f, 1.0f, 0.0f));
            }

            // Получить направление "вправо" (локальная ось X) в мировых координатах
            glm::vec3 getRight() const {
                glm::quat q = glm::quat(glm::vec3(
                    glm::radians(rotation.x),
                    glm::radians(rotation.y),
                    glm::radians(rotation.z)
                ));

                // Поворачиваем вектор (1,0,0) кватернионом
                return glm::normalize(q * glm::vec3(1.0f, 0.0f, 0.0f));
            }

            // Альтернативная реализация через матрицу поворота (без кватерниона)
            glm::vec3 getForwardMatrix() const {
                glm::mat4 rotMat = glm::mat4(1.0f);
                rotMat = glm::rotate(rotMat, glm::radians(rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
                rotMat = glm::rotate(rotMat, glm::radians(rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
                rotMat = glm::rotate(rotMat, glm::radians(rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));

                // Направление вперед - это 3-й столбец матрицы (для OpenGL, где ось Z - вперед)
                return glm::normalize(glm::vec3(rotMat[2][0], rotMat[2][1], rotMat[2][2]));
            }

            glm::vec3 getUpMatrix() const {
                glm::mat4 rotMat = glm::mat4(1.0f);
                rotMat = glm::rotate(rotMat, glm::radians(rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
                rotMat = glm::rotate(rotMat, glm::radians(rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
                rotMat = glm::rotate(rotMat, glm::radians(rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));

                // Направление вверх - это 2-й столбец матрицы
                return glm::normalize(glm::vec3(rotMat[1][0], rotMat[1][1], rotMat[1][2]));
            }
            void updateMatrices() {
                // Пока оставляем пустым, так как getMatrix() и так считает всё актуальное.
                // Это нужно, чтобы PhysicsSystem не ругалась на отсутствие метода.
            }
            // Повернуть объект в направлении вектора (lookAt)
            void lookAt(const glm::vec3& target, const glm::vec3& up = glm::vec3(0.0f, 1.0f, 0.0f)) {
                glm::vec3 direction = glm::normalize(target - position);

                // Вычисляем матрицу lookAt и извлекаем углы Эйлера
                glm::mat4 view = glm::lookAt(glm::vec3(0.0f), direction, up);
                glm::quat q = glm::quat_cast(view);

                // Конвертируем кватернион обратно в углы Эйлера (в градусах)
                glm::vec3 euler = glm::eulerAngles(q);
                rotation.x = glm::degrees(euler.x);
                rotation.y = glm::degrees(euler.y);
                rotation.z = glm::degrees(euler.z);
            }

        };
    }
}