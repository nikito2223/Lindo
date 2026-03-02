// Model.h
#pragma once

#include "core/OGL.h"
#include <glm/glm.hpp>
#include <string>
#include <vector>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <Graphics/core/Shader.h>
#include <Graphics/core/mesh.h>
#include <objects/transform.h>

class Model {
public:
    Model(const std::string& path);
    void Draw(Shader& shader);

    // Трансформация всей модели
    Transform transform;

    int getTriangleCount() const {
        int total = 0;
        for (const auto& mesh : meshes) {
            total += mesh.getTriangleCount();
        }
        return total;
    }

private:
    std::vector<Mesh> meshes;
    std::string directory;

    void loadModel(const std::string& path);
    void processNode(aiNode* node, const aiScene* scene);
    Mesh processMesh(aiMesh* mesh, const aiScene* scene);

    std::vector<Texture> loadMaterialTextures(aiMaterial* mat,
        aiTextureType type,
        std::string typeName);
    unsigned int textureFromFile(const char* path, const std::string& directory);
};