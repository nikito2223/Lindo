// Model.cpp
#include "model.h"
#include <iostream>
#include <Graphics/core/Texture.h>

Model::Model(const std::string& path)
{
    loadModel(path);
}

void Model::Draw(Shader& shader)
{
    for (auto& mesh : meshes)
    {
        // Устанавливаем матрицу модели и нормальную матрицу для каждого меша
        glm::mat4 modelMatrix = transform.getMatrix(); // Трансформация всей модели
        shader.setMat4("model", modelMatrix);

        // Вычисляем нормальную матрицу (только вращение и масштаб)
        glm::mat3 normalMatrix = glm::mat3(glm::transpose(glm::inverse(modelMatrix)));
        shader.setMat3("normalMatrix", normalMatrix);

        // Рисуем меш (он НЕ устанавливает матрицы сам)
        mesh.Draw(shader);
    }
}

void Model::loadModel(const std::string& path)
{
    std::vector<char> fileData;
    try {
        fileData = CryptoUtils::decryptFileBinary(path);
    }
    catch (const std::exception& e) {
        std::cerr << "Failed to read encrypted model: " << path << " - " << e.what() << std::endl;
        return;
    }

    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFileFromMemory(
        fileData.data(), fileData.size(),
        aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_GenNormals,
        path.c_str() // передаём путь для определения формата по расширению
    );

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        std::cerr << "Assimp error: " << importer.GetErrorString() << std::endl;
        return;
    }

    directory = path.substr(0, path.find_last_of('/'));
    processNode(scene->mRootNode, scene);
}

void Model::processNode(aiNode* node, const aiScene* scene)
{
    // Обрабатываем все меши узла
    for (unsigned int i = 0; i < node->mNumMeshes; i++)
    {
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        meshes.push_back(processMesh(mesh, scene));
    }

    // Рекурсивно обрабатываем дочерние узлы
    for (unsigned int i = 0; i < node->mNumChildren; i++)
    {
        processNode(node->mChildren[i], scene);
    }
}

Mesh Model::processMesh(aiMesh* mesh, const aiScene* scene)
{
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    std::vector<Texture> textures;

    // Обрабатываем вершины
    for (unsigned int i = 0; i < mesh->mNumVertices; i++)
    {
        Vertex vertex;

        // Позиция
        vertex.Position = glm::vec3(
            mesh->mVertices[i].x,
            mesh->mVertices[i].y,
            mesh->mVertices[i].z
        );

        // Нормали
        vertex.Normal = glm::vec3(
            mesh->mNormals[i].x,
            mesh->mNormals[i].y,
            mesh->mNormals[i].z
        );

        // Текстурные координаты
        if (mesh->mTextureCoords[0])
        {
            vertex.TexCoords = glm::vec2(
                mesh->mTextureCoords[0][i].x,
                mesh->mTextureCoords[0][i].y
            );
        }
        else
        {
            vertex.TexCoords = glm::vec2(0.0f, 0.0f);
        }

        // Тангенты и битангенты
        if (mesh->mTangents)
        {
            vertex.Tangent = glm::vec3(
                mesh->mTangents[i].x,
                mesh->mTangents[i].y,
                mesh->mTangents[i].z
            );

            vertex.Bitangent = glm::vec3(
                mesh->mBitangents[i].x,
                mesh->mBitangents[i].y,
                mesh->mBitangents[i].z
            );
        }

        vertices.push_back(vertex);
    }

    // Обрабатываем индексы
    for (unsigned int i = 0; i < mesh->mNumFaces; i++)
    {
        aiFace face = mesh->mFaces[i];
        for (unsigned int j = 0; j < face.mNumIndices; j++)
            indices.push_back(face.mIndices[j]);
    }

    // Обрабатываем материал
    if (mesh->mMaterialIndex >= 0)
    {
        aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];

        // Диффузные текстуры
        std::vector<Texture> diffuseMaps = loadMaterialTextures(material,
            aiTextureType_DIFFUSE, "texture_diffuse");
        textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());

        // Спекулярные текстуры
        std::vector<Texture> specularMaps = loadMaterialTextures(material,
            aiTextureType_SPECULAR, "texture_specular");
        textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());

        // Нормальные текстуры
        std::vector<Texture> normalMaps = loadMaterialTextures(material,
            aiTextureType_HEIGHT, "texture_normal");
        textures.insert(textures.end(), normalMaps.begin(), normalMaps.end());
    }

    // Создаем меш с единичной трансформацией
    return Mesh(vertices, indices, textures, Transform());
}

std::vector<Texture> Model::loadMaterialTextures(aiMaterial* mat,
    aiTextureType type,
    std::string typeName)
{
    std::vector<Texture> textures;
    for (unsigned int i = 0; i < mat->GetTextureCount(type); i++)
    {
        aiString str;
        mat->GetTexture(type, i, &str);

        // Проверяем, не загружена ли текстура уже
        bool skip = false;
        for (unsigned int j = 0; j < textures.size(); j++)
        {
            if (std::strcmp(textures[j].path.data(), str.C_Str()) == 0)
            {
                textures.push_back(textures[j]);
                skip = true;
                break;
            }
        }

        if (!skip)
        {
            Texture texture;
            texture.id = textureFromFile(str.C_Str(), directory);
            texture.type = typeName;
            texture.path = str.C_Str();
            textures.push_back(texture);
        }
    }
    return textures;
}

unsigned int Model::textureFromFile(const char* path, const std::string& directory)
{
    std::string filename = std::string(path);
    filename = directory + '/' + filename;
    return loadTexture(filename);  // теперь передаётся std::string
}