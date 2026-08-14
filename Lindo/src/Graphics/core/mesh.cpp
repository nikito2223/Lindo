// Mesh.cpp
#include "mesh.h"

Mesh::Mesh(
    const std::vector<Vertex>& vertices,
    const std::vector<unsigned int>& indices,
    const std::vector<Texture>& textures
)
    : vertices(vertices), indices(indices), textures(textures)
{
    setupMesh();
    calculateBoundingBox();  // автоматически вычисляем при создании
}

void Mesh::setupMesh()
{
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER,
        vertices.size() * sizeof(Vertex),
        vertices.data(),
        GL_STATIC_DRAW
    );

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
        indices.size() * sizeof(unsigned int),
        indices.data(),
        GL_STATIC_DRAW
    );

    // Vertex attributes
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Normal));

    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, TexCoords));

    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Tangent));

    glEnableVertexAttribArray(4);
    glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Bitangent));

    glBindVertexArray(0);
}

void Mesh::Draw(Shader& shader)
{
    unsigned int diffuseNr = 1;
    unsigned int specularNr = 1;

    for (unsigned int i = 0; i < textures.size(); i++) {
        glActiveTexture(GL_TEXTURE0 + i);
        std::string number;
        std::string name = textures[i].type;
        if (name == "texture_diffuse")
            number = std::to_string(diffuseNr++);
        else if (name == "texture_specular")
            number = std::to_string(specularNr++);

        shader.setInt(("material." + name + number).c_str(), i);
        glBindTexture(GL_TEXTURE_2D, textures[i].id);
    }

    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);

    glActiveTexture(GL_TEXTURE0);
}

void Mesh::calculateBoundingBox()
{
    if (vertices.empty()) {
        hasBBox = false;
        hasBSphere = false;
        return;
    }

    float minX = std::numeric_limits<float>::max();
    float minY = std::numeric_limits<float>::max();
    float minZ = std::numeric_limits<float>::max();
    float maxX = std::numeric_limits<float>::lowest();
    float maxY = std::numeric_limits<float>::lowest();
    float maxZ = std::numeric_limits<float>::lowest();

    // Находим min/max координаты вершин
    for (const auto& vertex : vertices) {
        minX = std::min(minX, vertex.Position.x);
        minY = std::min(minY, vertex.Position.y);
        minZ = std::min(minZ, vertex.Position.z);
        maxX = std::max(maxX, vertex.Position.x);
        maxY = std::max(maxY, vertex.Position.y);
        maxZ = std::max(maxZ, vertex.Position.z);
    }

    bboxMin = glm::vec3(minX, minY, minZ);
    bboxMax = glm::vec3(maxX, maxY, maxZ);
    hasBBox = true;

    // Вычисляем bounding sphere
    bsphereCenter = (bboxMin + bboxMax) * 0.5f;
    bsphereRadius = glm::length(bboxMax - bboxMin) * 0.5f;
    hasBSphere = true;

    // Опционально: вывод отладочной информации
    std::cout << "Mesh BBox: min(" << minX << ", " << minY << ", " << minZ
        << "), max(" << maxX << ", " << maxY << ", " << maxZ
        << "), radius: " << bsphereRadius << std::endl;
}