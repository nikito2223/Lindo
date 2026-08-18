#pragma once
#include <Component/GameObject/GameObject.h>
#include <Component/Component.h>
#include <Graphics/core/mesh.h>
#include <Graphics/core/model.h>
#include <Graphics/core/Shader.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <Component/Graphics/Material/Material.h>
#include <Debug/DebugLogger.h>
#include <utility>

namespace Lindo {
    namespace Components {
        namespace Physics {

            class MeshRenderer : public Lindo::World::Component {
            public:
                Lindo::Graphics::Mesh* mesh = nullptr;
                Lindo::Graphics::Model* model = nullptr;
                Lindo::Graphics::Material* material = nullptr;

                // Флаг активности компонента
                bool enabled = true;

                // Bounding box
                glm::vec3 bboxMin{ 0.0f };
                glm::vec3 bboxMax{ 0.0f };
                bool hasBBox = false;

                // Bounding sphere
                glm::vec3 bsphereCenter{ 0.0f };
                float bsphereRadius = 0.0f;
                bool hasBSphere = false;

                MeshRenderer() = default;

                MeshRenderer(Lindo::Graphics::Mesh* m) { setMesh(m); }
                MeshRenderer(Lindo::Graphics::Model* m) { setModel(m); }

                bool IsEnabled() const { return enabled; }

                // Сеттеры с автоматическим пересчетом Bounding Box
                void setMesh(Lindo::Graphics::Mesh* m) {
                    mesh = m;
                    model = nullptr;
                    calculateBoundingBox();
                }

                void setModel(Lindo::Graphics::Model* m) {
                    model = m;
                    mesh = nullptr;
                    calculateBoundingBox();
                }

                static Lindo::Graphics::Material* GetDefaultMaterial() {
                    static Lindo::Graphics::Material defaultMat(0, 0, 32.0f, false);
                    defaultMat.useTexture = false;
                    defaultMat.color = glm::vec3(1.0f); // Белый цвет
                    return &defaultMat;
                }

                void OnDraw(Lindo::Graphics::Shader& shader) override {
                    if (!gameObject || !IsEnabled()) return;

                    if (!mesh && !model) {
                        LOG_WARN("MeshRenderer on '" + gameObject->getName() + "' has no mesh/model assigned - skipping draw!");
                        return;
                    }

                    // Получаем мировую матрицу GameObject'а (с учётом родителей)
                    glm::mat4 worldMatrix = gameObject->getWorldMatrix();

                    Lindo::Graphics::Material* currentMaterial = material ? material : GetDefaultMaterial();
                    currentMaterial->apply(shader);

                    if (mesh) {
                        // Для одиночного меша передаём мировую матрицу, чтобы тени и геометрия совпадали
                        shader.setMat4("model", worldMatrix);
                        glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(worldMatrix)));
                        shader.setMat3("normalMatrix", normalMatrix);
                        mesh->Draw(shader);
                    }
                    else if (model) {
                        // 🔥 Передаем мировую матрицу GameObject'а в модель!
                        model->Draw(shader, worldMatrix);
                    }
                }

                void calculateBoundingBox() {
                    if (mesh) {
                        calculateMeshBoundingBox();
                    }
                    else if (model) {
                        calculateModelBoundingBox();
                    }
                }

                std::pair<glm::vec3, glm::vec3> getTransformedBBox() const {
                    if (!hasBBox || !gameObject) return { glm::vec3(0), glm::vec3(0) };
                    glm::mat4 transformMatrix = gameObject->transform.getMatrix();

                    glm::vec3 transformedMin = glm::vec3(transformMatrix * glm::vec4(bboxMin, 1.0f));
                    glm::vec3 transformedMax = glm::vec3(transformMatrix * glm::vec4(bboxMax, 1.0f));

                    glm::vec3 size = bboxMax - bboxMin;
                    float maxSize = glm::max(glm::max(size.x, size.y), size.z);
                    glm::vec3 center = (transformedMin + transformedMax) * 0.5f;

                    transformedMin = center - glm::vec3(maxSize * 0.5f);
                    transformedMax = center + glm::vec3(maxSize * 0.5f);

                    return { transformedMin, transformedMax };
                }

                std::pair<glm::vec3, float> getTransformedBSphere() const {
                    if (!hasBSphere || !gameObject) return { glm::vec3(0), 0.0f };
                    glm::mat4 transformMatrix = gameObject->transform.getMatrix();
                    glm::vec3 transformedCenter = glm::vec3(transformMatrix * glm::vec4(bsphereCenter, 1.0f));

                    float scaleFactor = glm::max(glm::max(gameObject->transform.scale.x,
                        gameObject->transform.scale.y),
                        gameObject->transform.scale.z);

                    float transformedRadius = bsphereRadius * scaleFactor;
                    return { transformedCenter, transformedRadius };
                }

            private:
                void calculateMeshBoundingBox();
                void calculateModelBoundingBox();
            };

        }
    }
}