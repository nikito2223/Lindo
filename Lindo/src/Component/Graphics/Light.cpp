#include "Light.h"
#include <Physics/Collider/BoxCollider.h>
#include <Physics/Collider/CapsuleCollider.h>
#include <Component/Physhcs/MeshRenderer.h>

void Light::ComputeLightMatrices(glm::mat4& proj, glm::mat4& view, const std::vector<GameObject*>& objects) const {
    if (type == LightType::Directional) {
        // 1. Вычисляем AABB всех объектов, отбрасывающих тени
        glm::vec3 minAABB(FLT_MAX), maxAABB(-FLT_MAX);
        bool hasObjects = false;
        for (auto* obj : objects) {
            if (!obj || !obj->castsShadows) continue;

            glm::vec3 pos = obj->transform.position;
            glm::vec3 halfSize(1.0f); // запас, если нет коллайдера

            if (obj->getComponent<Collider>()) {
                if (auto* box = obj->getComponent<BoxCollider>()) {
                    halfSize = box->getSize() * 0.5f;
                }
                else if (auto* capsule = obj->getComponent<CapsuleCollider>()) {
                    halfSize = glm::vec3(capsule->getRadius(),
                        capsule->getHeight() * 0.5f + capsule->getRadius(),
                        capsule->getRadius());
                }
            }

            glm::vec3 objMin = pos - halfSize;
            glm::vec3 objMax = pos + halfSize;

            minAABB = glm::min(minAABB, objMin);
            maxAABB = glm::max(maxAABB, objMax);
            hasObjects = true;
        }

        // Если объектов нет – используем значения по умолчанию (можно оставить статику)
        if (!hasObjects) {
            proj = glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, 1.0f, 20.0f);
            view = glm::lookAt(direction * -10.0f, glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
            return;
        }

        // 2. Определяем центр и размер сцены
        glm::vec3 center = (minAABB + maxAABB) * 0.5f;
        glm::vec3 size = maxAABB - minAABB;
        float maxExtent = glm::max(size.x, glm::max(size.y, size.z));

        // 3. Нормализуем направление света
        glm::vec3 lightDir = glm::normalize(direction);
        float distance = maxExtent * 2.0f; // запас, чтобы камера была снаружи

        // Позиция камеры света – позади сцены вдоль направления света
        glm::vec3 lightPos = center - lightDir * distance;

        // Матрица вида: смотрим из lightPos на центр
        view = glm::lookAt(lightPos, center, glm::vec3(0.0f, 1.0f, 0.0f));

        // 4. Вычисляем границы AABB в пространстве вида
        std::vector<glm::vec3> corners = {
            minAABB,
            glm::vec3(maxAABB.x, minAABB.y, minAABB.z),
            glm::vec3(minAABB.x, maxAABB.y, minAABB.z),
            glm::vec3(minAABB.x, minAABB.y, maxAABB.z),
            glm::vec3(maxAABB.x, maxAABB.y, minAABB.z),
            glm::vec3(maxAABB.x, minAABB.y, maxAABB.z),
            glm::vec3(minAABB.x, maxAABB.y, maxAABB.z),
            maxAABB
        };

        float minX = FLT_MAX, maxX = -FLT_MAX;
        float minY = FLT_MAX, maxY = -FLT_MAX;
        float minZ = FLT_MAX, maxZ = -FLT_MAX;

        for (const auto& v : corners) {
            glm::vec4 vLight = view * glm::vec4(v, 1.0f);
            minX = glm::min(minX, vLight.x);
            maxX = glm::max(maxX, vLight.x);
            minY = glm::min(minY, vLight.y);
            maxY = glm::max(maxY, vLight.y);
            minZ = glm::min(minZ, vLight.z);
            maxZ = glm::max(maxZ, vLight.z);
        }

        // 5. Корректные near/far как расстояния до плоскостей
        float near = -maxZ;   // ближайшая точка (наибольший Z → наименьшее расстояние)
        float far = -minZ;   // дальняя точка (наименьший Z → наибольшее расстояние)

        // Добавляем небольшой запас, чтобы избежать обрезания точно по граням
        float padding = 1.0f;
        near = glm::max(0.1f, near - padding);
        far = far + padding;

        // Ортографическая проекция
        proj = glm::ortho(minX - padding, maxX + padding,
            minY - padding, maxY + padding,
            near, far);
    }
    else if (type == LightType::Spot) {
        proj = glm::perspective(glm::radians(outerCutOff * 2.0f), 1.0f, 1.0f, farPlane);
        view = glm::lookAt(getPosition(), getPosition() + direction, glm::vec3(0.0f, 1.0f, 0.0f));
    }
}

void Light::DrawRadius(Shader& shader) const {
    // Для направленного света радиус не определён
    if (type == LightType::Directional) return;

    float r = GetRadius();
    if (r <= 0.0f) return;

    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, getPosition());
    // debugMesh изначально имеет радиус 0.2, масштабируем до нужного радиуса
    float scale = r / 0.2f;
    model = glm::scale(model, glm::vec3(scale));

    shader.setMat4("model", model);
    // Используем цвет света, но полупрозрачный
    shader.setVec3("color", color * intensity);
    shader.setFloat("alpha", 0.15f); // очень прозрачный, чтобы не заслонять сцену

    // Временно отключаем запись в буфер глубины, чтобы сфера не перекрывала объекты
    // (но можно и оставить, зависит от желаемого эффекта)
    //glDepthMask(GL_FALSE);
    //debugMesh->Draw(shader);
    //glDepthMask(GL_TRUE);
}

void Light::SetRadius(float newRadius)
{
    radius = newRadius;
    constant = 1.0f;

    // Настраиваем линейное и квадратичное затухание под нужный радиус
    linear = 4.5f / radius;
    quadratic = 75.0f / (radius * radius);

    std::cout << radius << linear << quadratic << std::endl;
}

void Light::RenderShadows(const std::vector<GameObject*>& objects,
    int screenWidth,
    int screenHeight)
{
    // Защита от рекурсии
    static std::unordered_map<Light*, bool> renderingNow;
    if (renderingNow[this]) {
        std::cerr << "WARNING: Recursive call to RenderShadows detected for light: " << name << std::endl;
        return;
    }
    renderingNow[this] = true;

    if (!enabled || !castShadows || !shadowMap || !shadowDepthShader) {
        renderingNow[this] = false;
        return;
    }

    // ============================
    // Directional / Spot
    // ============================
    if (type == LightType::Directional || type == LightType::Spot)
    {
        glm::mat4 proj, view;

        // ВАЖНО: Создаем копию вектора объектов, чтобы избежать проблем с модификацией
        std::vector<GameObject*> objectsCopy = objects;
        ComputeLightMatrices(proj, view, objectsCopy);

        lightSpaceMatrix = proj * view;

        shadowMap->BeginRender();

        shadowDepthShader->use();
        shadowDepthShader->setMat4("lightSpaceMatrix", lightSpaceMatrix);

        for (auto* obj : objects)
        {
            if (!obj->castsShadows || !obj) continue;

            shadowDepthShader->setMat4("model",
                obj->transform.getMatrixQuat());

            auto* meshRenderer = obj->getComponent<MeshRenderer>();
            if (meshRenderer == nullptr) continue;
            if (meshRenderer->mesh) {
                meshRenderer->mesh->Draw(*shadowDepthShader);  // или meshRenderer->Draw(*shadowDepthShader);
            }
            else if (meshRenderer->model)
                meshRenderer->model->Draw(*shadowDepthShader);
        }

        shadowMap->EndRender(screenWidth, screenHeight);
    }

    // ============================
    // Point Light (Cubemap)
    // ============================
    else if (type == LightType::Point)
    {
        float aspect = 1.0f;
        glm::mat4 shadowProj =
            glm::perspective(glm::radians(90.0f),
                aspect,
                1.0f,
                farPlane);
        std::vector<glm::mat4> shadowTransforms;

        shadowTransforms.push_back(shadowProj *
            glm::lookAt(getPosition(),
                getPosition() + glm::vec3(1, 0, 0),
                glm::vec3(0, -1, 0)));

        shadowTransforms.push_back(shadowProj *
            glm::lookAt(getPosition(),
                getPosition() + glm::vec3(-1, 0, 0),
                glm::vec3(0, -1, 0)));

        shadowTransforms.push_back(shadowProj *
            glm::lookAt(getPosition(),
                getPosition() + glm::vec3(0, 1, 0),
                glm::vec3(0, 0, 1)));

        shadowTransforms.push_back(shadowProj *
            glm::lookAt(getPosition(),
                getPosition() + glm::vec3(0, -1, 0),
                glm::vec3(0, 0, -1)));

        shadowTransforms.push_back(shadowProj *
            glm::lookAt(getPosition(),
                getPosition() + glm::vec3(0, 0, 1),
                glm::vec3(0, -1, 0)));

        shadowTransforms.push_back(shadowProj *
            glm::lookAt(getPosition(),
                getPosition() + glm::vec3(0, 0, -1),
                glm::vec3(0, -1, 0)));

        shadowMap->BeginRender();

        shadowDepthShader->use();

        for (int i = 0; i < 6; ++i)
        {
            shadowDepthShader->setMat4(
                "shadowMatrices[" + std::to_string(i) + "]",
                shadowTransforms[i]);
        }

        shadowDepthShader->setVec3("lightPos", getPosition());
        shadowDepthShader->setFloat("farPlane", farPlane);

        for (auto* obj : objects)
        {
            if (!obj->castsShadows) continue;

            shadowDepthShader->setMat4("model",
                obj->transform.getMatrixQuat());
            auto* meshRenderer = obj->getComponent<MeshRenderer>();

            if (meshRenderer == nullptr) continue;

            if (meshRenderer->mesh) {
                meshRenderer->mesh->Draw(*shadowDepthShader);  // или meshRenderer->Draw(*shadowDepthShader);
            }
            else if (meshRenderer->model)
                meshRenderer->model->Draw(*shadowDepthShader);
        }

        shadowMap->EndRender(screenWidth, screenHeight);
    }
}