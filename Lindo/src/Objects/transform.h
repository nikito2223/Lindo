#define GLM_ENABLE_EXPERIMENTAL
#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

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

};