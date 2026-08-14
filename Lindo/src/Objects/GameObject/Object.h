#pragma once
#include <glm/glm.hpp>
#include <memory>
#include <Graphics/core/mesh.h>
#include <Graphics/core/model.h>

class Object {
public:
    Mesh* mesh = nullptr;
    Model* model = nullptr;

    // Bounding box (локальный!)
    glm::vec3 bboxMin{ 0.0f };
    glm::vec3 bboxMax{ 0.0f };
    bool hasBBox = false;

    // Bounding sphere
    glm::vec3 bsphereCenter{ 0.0f };
    float bsphereRadius = 0.0f;
    bool hasBSphere = false;

    bool castsShadows = true;
    bool receivesShadows = true;

    Object() = default;
    Object(Mesh* m) : mesh(m) { calculateBoundingBox(); }
    Object(Model* m) : model(m) { calculateBoundingBox(); }

    virtual void Draw(Shader& shader, const Transform& transform) {

        if (mesh) {
            //glm::mat4 objectMatrix = transform.getMatrix();
            ///*glm::mat4 meshMatrix = mesh.*/
            ///*glm::mat4 combined = objectMatrix * meshMatrix;*/

            ///*shader.setMat4("model", combined);*/

            //glm::mat3 normalMatrix =
            //    glm::transpose(glm::inverse(glm::mat3(combined)));

            //shader.setMat3("normalMatrix", normalMatrix);

            //mesh->Draw(shader);
        }
        else if (model) {
            model->transform = transform;
            model->Draw(shader);
        }
    }

    // ---- Bounding Box с учётом transform GameObject ----
    std::pair<glm::vec3, glm::vec3>
        getTransformedBBox(const Transform& transform) const {

        if (!hasBBox)
            return { glm::vec3(0), glm::vec3(0) };

        glm::mat4 matrix = transform.getMatrix();

        glm::vec3 min =
            glm::vec3(matrix * glm::vec4(bboxMin, 1.0f));
        glm::vec3 max =
            glm::vec3(matrix * glm::vec4(bboxMax, 1.0f));

        return { min, max };
    }

    std::pair<glm::vec3, float>
        getTransformedBSphere(const Transform& transform) const {

        if (!hasBSphere)
            return { glm::vec3(0), 0.0f };

        glm::mat4 matrix = transform.getMatrix();

        glm::vec3 center =
            glm::vec3(matrix * glm::vec4(bsphereCenter, 1.0f));

        float scaleFactor = glm::max(
            glm::max(transform.scale.x, transform.scale.y),
            transform.scale.z
        );

        float radius = bsphereRadius * scaleFactor;

        return { center, radius };
    }

    virtual ~Object() {}

private:
    void calculateBoundingBox() {
        if (mesh) calculateMeshBoundingBox();
        else if (model) calculateModelBoundingBox();
    }

    void calculateMeshBoundingBox();
    void calculateModelBoundingBox();
};