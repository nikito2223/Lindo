// Model.cpp
#include "model.h"
#include <iostream>
#include <algorithm>
#include <Core/AssetManager.h> // Проверь правильность пути к AssetManager.h!

namespace Lindo {
    namespace Graphics {

        Model::Model(const std::string& path)
        {
            loadModel(path);
        }

        void Model::Draw(Shader& shader, const glm::mat4& parentTransform)
        {
            // 🔥 Комбинируем матрицу GameObject'а и локальный трансформ самой модели
            glm::mat4 finalModelMatrix = parentTransform * transform.getMatrix();
            shader.setMat4("model", finalModelMatrix);

            glm::mat3 normalMatrix = glm::mat3(glm::transpose(glm::inverse(finalModelMatrix)));
            shader.setMat3("normalMatrix", normalMatrix);

            for (auto& mesh : meshes)
            {
                mesh.Draw(shader);
            }
        }

        void Model::loadModel(const std::string& path)
        {
            Assimp::Importer importer;
            const aiScene* scene = importer.ReadFile(
                path,
                aiProcess_Triangulate |
                aiProcess_GenNormals |
                aiProcess_CalcTangentSpace
            );

            if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
                std::cerr << "Assimp error: " << importer.GetErrorString() << std::endl;
                return;
            }

            directory = path.substr(0, path.find_last_of("/\\"));
            processNode(scene->mRootNode, scene);
        }

        void Model::processNode(aiNode* node, const aiScene* scene)
        {
            for (unsigned int i = 0; i < node->mNumMeshes; i++)
            {
                aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
                meshes.push_back(processMesh(mesh, scene));
            }

            for (unsigned int i = 0; i < node->mNumChildren; i++)
            {
                processNode(node->mChildren[i], scene);
            }
        }

        Mesh Model::processMesh(aiMesh* mesh, const aiScene* scene)
        {
            std::vector<Mesh::Vertex> vertices;
            std::vector<unsigned int> indices;
            std::vector<Mesh::Texture> textures;

            vertices.reserve(mesh->mNumVertices);
            indices.reserve(mesh->mNumFaces * 3);

            // Обработка вершин
            for (unsigned int i = 0; i < mesh->mNumVertices; i++)
            {
                Mesh::Vertex vertex;
                vertex.Position = glm::vec3(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z);
                vertex.Normal = glm::vec3(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z);

                if (mesh->mTextureCoords[0]) {
                    vertex.TexCoords = glm::vec2(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y);
                }
                else {
                    vertex.TexCoords = glm::vec2(0.0f, 0.0f);
                }

                if (mesh->mTangents) {
                    vertex.Tangent = glm::vec3(mesh->mTangents[i].x, mesh->mTangents[i].y, mesh->mTangents[i].z);
                    vertex.Bitangent = glm::vec3(mesh->mBitangents[i].x, mesh->mBitangents[i].y, mesh->mBitangents[i].z);
                }

                vertices.push_back(vertex);
            }

            // Обработка индексов
            for (unsigned int i = 0; i < mesh->mNumFaces; i++)
            {
                aiFace face = mesh->mFaces[i];
                for (unsigned int j = 0; j < face.mNumIndices; j++)
                    indices.push_back(face.mIndices[j]);
            }

            // Обработка материалов/текстур
            if (mesh->mMaterialIndex >= 0)
            {
                aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];

                auto diffuseMaps = loadMaterialTextures(material, aiTextureType_DIFFUSE, "texture_diffuse");
                textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());

                auto specularMaps = loadMaterialTextures(material, aiTextureType_SPECULAR, "texture_specular");
                textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());

                auto normalMaps = loadMaterialTextures(material, aiTextureType_HEIGHT, "texture_normal");
                textures.insert(textures.end(), normalMaps.begin(), normalMaps.end());
            }

            return Mesh(vertices, indices, textures);
        }

        std::vector<Mesh::Texture> Model::loadMaterialTextures(aiMaterial* mat, aiTextureType type, std::string typeName)
        {
            std::vector<Mesh::Texture> textures;
            for (unsigned int i = 0; i < mat->GetTextureCount(type); i++)
            {
                aiString str;
                mat->GetTexture(type, i, &str);

                Mesh::Texture texture;
                // Запрашиваем ID текстуры у AssetManager
                texture.id = textureFromFile(str.C_Str(), this->directory);
                texture.type = typeName;
                texture.path = str.C_Str();
                textures.push_back(texture);
            }
            return textures;
        }

        unsigned int Model::textureFromFile(const char* path, const std::string& directory)
        {
            std::string filename = directory + '/' + std::string(path);

            // 🔥 ФИКС: Используем метод твоего AssetManager, который гарантирует синглтон-доступ
            return Lindo::AssetManager::get().loadTexture(filename);
        }
    }
}