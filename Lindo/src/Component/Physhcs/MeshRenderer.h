#pragma once
#include <Component/GameObject/GameObject.h>
#include <Component/Component.h>
#include <Graphics/core/mesh.h>
#include <Graphics/core/model.h>
#include <Graphics/core/Shader.h>
#include <glm/glm.hpp>

class MeshRenderer : public Component {
public:
    Mesh* mesh = nullptr;
    Model* model = nullptr;

    // Bounding box
    glm::vec3 bboxMin{ 0.0f };
    glm::vec3 bboxMax{ 0.0f };
    bool hasBBox = false;

    // Bounding sphere
    glm::vec3 bsphereCenter{ 0.0f };
    float bsphereRadius = 0.0f;
    bool hasBSphere = false;

    MeshRenderer() = default;
    MeshRenderer(Mesh* m) : mesh(m) { calculateBoundingBox(); }
    MeshRenderer(Model* m) : model(m) { calculateBoundingBox(); }

    void OnDraw(Shader& shader) override {

        if (!owner) return;
        
        glm::mat4 modelMatrix = owner->transform.getMatrix();

        shader.setMat4("model", modelMatrix);

        glm::mat3 normalMatrix =
            glm::transpose(glm::inverse(glm::mat3(modelMatrix)));

        shader.setMat3("normalMatrix", normalMatrix);

        if (mesh)
            mesh->Draw(shader);

        else if (model)
            model->Draw(shader);
    }

    void calculateBoundingBox() {
        if (mesh) calculateMeshBoundingBox();
        else if (model) calculateModelBoundingBox();
    }

    std::pair<glm::vec3, glm::vec3> getTransformedBBox() const {
        if (!hasBBox) return { glm::vec3(0), glm::vec3(0) };
        glm::mat4 transformMatrix = owner->transform.getMatrix();

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
        glm::mat4 transformMatrix = owner->transform.getMatrix();
        glm::vec3 transformedCenter = glm::vec3(transformMatrix * glm::vec4(bsphereCenter, 1.0f));
        float scaleFactor = glm::max(glm::max(owner->transform.scale.x,
            owner->transform.scale.y),
            owner->transform.scale.z);
        float transformedRadius = bsphereRadius * scaleFactor;
        return { transformedCenter, transformedRadius };
    }

private:
    void calculateMeshBoundingBox();
    void calculateModelBoundingBox();
};