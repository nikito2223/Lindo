#include "mesh.h"

namespace Lindo {
    namespace Graphics {

        Mesh::Mesh(const std::vector<Vertex>& vertices,
            const std::vector<unsigned int>& indices,
            const std::vector<Texture>& textures)
            : vertices(vertices), indices(indices), textures(textures)
        {
            setupMesh();
            calculateBoundingBox();
        }

        void Mesh::setupMesh()
        {
            glGenVertexArrays(1, &VAO);
            glGenBuffers(1, &VBO);
            glGenBuffers(1, &EBO);

            glBindVertexArray(VAO);

            glBindBuffer(GL_ARRAY_BUFFER, VBO);
            glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);

            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

            // Attributes using offsetof safely
            glEnableVertexAttribArray(0);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, Position)));

            glEnableVertexAttribArray(1);
            glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, Normal)));

            glEnableVertexAttribArray(2);
            glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, TexCoords)));

            glEnableVertexAttribArray(3);
            glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, Tangent)));

            glEnableVertexAttribArray(4);
            glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, Bitangent)));

            glBindVertexArray(0);
        }

        void Mesh::Draw(Shader& shader)
        {
            unsigned int diffuseNr = 1;
            unsigned int specularNr = 1;
            unsigned int normalNr = 1;

            for (unsigned int i = 0; i < textures.size(); i++) {
                glActiveTexture(GL_TEXTURE0 + i);

                std::string number;
                const std::string& name = textures[i].type;

                if (name == "texture_diffuse")
                    number = std::to_string(diffuseNr++);
                else if (name == "texture_specular")
                    number = std::to_string(specularNr++);
                else if (name == "texture_normal")
                    number = std::to_string(normalNr++); // Добавлена поддержка нормалей в шейдер

                shader.setInt(("material." + name + number).c_str(), i);
                glBindTexture(GL_TEXTURE_2D, textures[i].id);
            }

            glBindVertexArray(VAO);
            glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(indices.size()), GL_UNSIGNED_INT, nullptr);
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

            glm::vec3 minBound(std::numeric_limits<float>::max());
            glm::vec3 maxBound(std::numeric_limits<float>::lowest());

            for (const auto& vertex : vertices) {
                minBound = glm::min(minBound, vertex.Position);
                maxBound = glm::max(maxBound, vertex.Position);
            }

            bboxMin = minBound;
            bboxMax = maxBound;
            hasBBox = true;

            bsphereCenter = (bboxMin + bboxMax) * 0.5f;
            bsphereRadius = glm::length(bboxMax - bboxMin) * 0.5f;
            hasBSphere = true;
        }
    }
}