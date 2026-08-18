#pragma once

#include "core/OGL.h"
#include <glm/glm.hpp>
#include <vector>
#include <string>
#include <limits>      // для numeric_limits
#include <iostream>    // для вывода отладки
#include <algorithm>   // для min/max
#include <Component/GameObject/transform.h>
#include "Shader.h"

namespace Lindo {
    namespace Graphics {

        class Mesh {
        public:

            struct Vertex {
                glm::vec3 Position;
                glm::vec3 Normal;
                glm::vec2 TexCoords;
                glm::vec3 Tangent;
                glm::vec3 Bitangent;
            };

            struct Texture {
                unsigned int id;
                std::string type; // texture_diffuse, texture_specular
                std::string path;
            };

            std::vector<Vertex> vertices;
            std::vector<unsigned int> indices;
            std::vector<Texture> textures;

            // Bounding box (локальный!)
            glm::vec3 bboxMin{ 0.0f };
            glm::vec3 bboxMax{ 0.0f };
            bool hasBBox = false;

            // Bounding sphere
            glm::vec3 bsphereCenter{ 0.0f };
            float bsphereRadius = 0.0f;
            bool hasBSphere = false;

            Mesh(const std::vector<Vertex>& vertices,
                const std::vector<unsigned int>& indices,
                const std::vector<Texture>& textures);

            void Draw(Lindo::Graphics::Shader& shader);

            size_t getTriangleCount() const {
                if (indices.empty()) {
                    return vertices.size() / 3;
                }
                return indices.size() / 3;
            }

            // Получить трансформированный bounding box с учетом transform объекта
            std::pair<glm::vec3, glm::vec3>
                getTransformedBBox(const Lindo::Math::Transform& transform) const {

                if (!hasBBox) return { glm::vec3(0), glm::vec3(0) };

                glm::mat4 matrix = transform.getMatrix();

                // Трансформируем все 8 углов box'а и находим новые min/max
                std::vector<glm::vec3> corners = {
                    glm::vec3(bboxMin.x, bboxMin.y, bboxMin.z),
                    glm::vec3(bboxMax.x, bboxMin.y, bboxMin.z),
                    glm::vec3(bboxMin.x, bboxMax.y, bboxMin.z),
                    glm::vec3(bboxMax.x, bboxMax.y, bboxMin.z),
                    glm::vec3(bboxMin.x, bboxMin.y, bboxMax.z),
                    glm::vec3(bboxMax.x, bboxMin.y, bboxMax.z),
                    glm::vec3(bboxMin.x, bboxMax.y, bboxMax.z),
                    glm::vec3(bboxMax.x, bboxMax.y, bboxMax.z)
                };

                glm::vec3 min = glm::vec3(std::numeric_limits<float>::max());
                glm::vec3 max = glm::vec3(std::numeric_limits<float>::lowest());

                for (const auto& corner : corners) {
                    glm::vec3 transformed = glm::vec3(matrix * glm::vec4(corner, 1.0f));
                    min = glm::min(min, transformed);
                    max = glm::max(max, transformed);
                }

                return { min, max };
            }

            // Получить трансформированную bounding sphere с учетом transform объекта
            std::pair<glm::vec3, float>
                getTransformedBSphere(const Lindo::Math::Transform& transform) const {

                if (!hasBSphere) return { glm::vec3(0), 0.0f };

                glm::mat4 matrix = transform.getMatrix();

                // Центр сферы трансформируется как точка
                glm::vec3 center = glm::vec3(matrix * glm::vec4(bsphereCenter, 1.0f));

                // Радиус масштабируется по максимальному масштабу
                float scaleFactor = glm::max(
                    glm::max(transform.scale.x, transform.scale.y),
                    transform.scale.z
                );

                float radius = bsphereRadius * scaleFactor;

                return { center, radius };
            }

            unsigned int GetVAO() const { return VAO; };

        private:
            unsigned int VAO, VBO, EBO;

            void setupMesh();
            void calculateBoundingBox();  // новый метод
        };
    }
}