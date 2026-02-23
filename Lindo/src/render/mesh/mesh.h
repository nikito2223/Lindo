#pragma once

#include "core/OGL.h"
#include <glm/glm.hpp>
#include <vector>
#include <string>
#include "shader/shader_s.h"
#include "Transform.h"   // <-- вставляем сюда

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

class Mesh {
public:
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    std::vector<Texture> textures;
    Transform transform; // Каждый меш имеет свою собственную трансформацию

    Mesh(const std::vector<Vertex>& vertices,
        const std::vector<unsigned int>& indices,
        const std::vector<Texture>& textures,
        const Transform& transform = Transform()); // По умолчанию пустая трансформация

    void Draw(Shader& shader);

private:
    unsigned int VAO, VBO, EBO;
    void setupMesh();
};
