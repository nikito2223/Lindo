#pragma once
#include <render/mesh/transform.h>
#include <render/mesh/mesh.h>
#include <utils/model.h>
#include <glm/glm.hpp>
#include <memory>
#include <render/Collision/Collider.h>  // или конкретные типы коллайдеров

class Object {
public:
    Mesh* mesh = nullptr;
    Model* model = nullptr;
    Transform transform;

    // Bounding box для отсечения
    glm::vec3 bboxMin{ 0.0f };
    glm::vec3 bboxMax{ 0.0f };
    bool hasBBox = false;

    // Для сферы (альтернатива bbox)
    glm::vec3 bsphereCenter{ 0.0f };
    float bsphereRadius = 0.0f;
    bool hasBSphere = false;
    bool castsShadows = true;
    bool receivesShadows = true;

    // Коллайдер объекта (если есть)
    std::shared_ptr<Collider> collider;
    bool hasCollider = false;

    Object() = default;
    Object(Mesh* m) : mesh(m) { calculateBoundingBox(); }
    Object(Model* m) : model(m) { calculateBoundingBox(); }

    virtual void Draw(Shader& shader) {
        if (mesh) {
            glm::mat4 objectMatrix = transform.getMatrix();
            glm::mat4 meshMatrix = mesh->transform.getMatrix();
            glm::mat4 combinedMatrix = objectMatrix * meshMatrix;

            shader.setMat4("model", combinedMatrix);

            glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(combinedMatrix)));
            shader.setMat3("normalMatrix", normalMatrix);

            mesh->Draw(shader);
        }
        else if (model) {
            model->transform = this->transform;
            model->Draw(shader);
        }
    }

    // Синхронизация коллайдера с трансформацией объекта
    virtual void updateCollider() {
        if (collider) {
            collider->getTransform().position = transform.position;
            collider->getTransform().rotation = transform.rotation;
            collider->getTransform().scale = transform.scale;
        }
    }

    void calculateBoundingBox() {
        if (mesh) calculateMeshBoundingBox();
        else if (model) calculateModelBoundingBox();
    }

    std::pair<glm::vec3, glm::vec3> getTransformedBBox() const {
        if (!hasBBox) return { glm::vec3(0), glm::vec3(0) };

        glm::mat4 transformMatrix = transform.getMatrix();
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
        if (!hasBSphere) return { glm::vec3(0), 0.0f };

        glm::mat4 transformMatrix = transform.getMatrix();
        glm::vec3 transformedCenter = glm::vec3(transformMatrix * glm::vec4(bsphereCenter, 1.0f));

        float scaleFactor = glm::max(glm::max(transform.scale.x, transform.scale.y), transform.scale.z);
        float transformedRadius = bsphereRadius * scaleFactor;

        return { transformedCenter, transformedRadius };
    }

    virtual ~Object() {}

private:
    void calculateMeshBoundingBox();
    void calculateModelBoundingBox();
};