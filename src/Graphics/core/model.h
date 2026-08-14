// Model.h
#pragma once

#include "core/OGL.h"
#include <glm/glm.hpp>
#include <string>
#include <vector>
#include <utility>
#include <limits>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <Graphics/core/Shader.h>
#include <Graphics/core/mesh.h>
#include <Component/GameObject/transform.h>

#include <Physics/Math/AABB.h>

namespace Lindo {
    namespace Graphics {

        class Model {
        public:
            Model(const std::string& path);
            void Draw(Shader& shader, const glm::mat4& parentTransform = glm::mat4(1.0f));

            // ������������� ���� ������
            Lindo::Math::Transform transform;

            std::vector<Mesh>& getMeshes() { return meshes; }

            // �������� ���������� ��� (��������, ������)
            Mesh* getMesh(size_t index) {
                if (index < meshes.size()) return &meshes[index];
                return nullptr;
            }

            size_t getTriangleCount() const {
                size_t total = 0;
                for (const auto& mesh : meshes) {
                    total += mesh.getTriangleCount();
                }
                return total;
            }

            Lindo::Math::AABB GetAABB() const {
                glm::vec3 min(std::numeric_limits<float>::max());
                glm::vec3 max(std::numeric_limits<float>::lowest());
            
                for (const auto& mesh : meshes) {
                    if (mesh.hasBBox) {
                        min = glm::min(min, mesh.bboxMin);
                        max = glm::max(max, mesh.bboxMax);
                    }
                }
                return Lindo::Math::AABB(min, max);
            }

            Lindo::Math::AABB getAABB() const { return GetAABB(); }

            // Старая версия через pair (чтобы ничего не сломать, если где-то используется)
            std::pair<glm::vec3, glm::vec3> getLocalAABB() const {
                Lindo::Math::AABB aabb = GetAABB();
                return { aabb.min, aabb.max };
            }

        private:
            std::vector<Mesh> meshes;
            std::string directory;

            void loadModel(const std::string& path);
            void processNode(aiNode* node, const aiScene* scene);
            Mesh processMesh(aiMesh* mesh, const aiScene* scene);

            std::vector<Lindo::Graphics::Mesh::Texture> loadMaterialTextures(aiMaterial* mat,
                aiTextureType type,
                std::string typeName);
            unsigned int textureFromFile(const char* path, const std::string& directory);
        };
    }
}