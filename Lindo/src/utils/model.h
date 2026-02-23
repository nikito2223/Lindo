// Model.h
#pragma once

#include "core/OGL.h"
#include <glm/glm.hpp>
#include <string>
#include <vector>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <render/mesh/mesh.h>

class Model {
public:
    Model(const std::string& path);
    void Draw(Shader& shader);

    // Трансформация всей модели
    Transform transform;

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