#include "mesh.h"
#include <Core/Types/Settings.h>
#include <unordered_set>

namespace Lindo {
    namespace Graphics {
        namespace {
            void CheckMeshOpenGLError(const std::string& operation, unsigned int vao,
                unsigned int ebo, const std::vector<unsigned int>& meshIndices, std::size_t vertexCount) {
                const std::size_t indexCount = meshIndices.size();
                GLenum error = GL_NO_ERROR;
                while ((error = glGetError()) != GL_NO_ERROR) {
                    const char* errorName = "GL_UNKNOWN_ERROR";
                    switch (error) {
                    case GL_INVALID_ENUM: errorName = "GL_INVALID_ENUM"; break;
                    case GL_INVALID_VALUE: errorName = "GL_INVALID_VALUE"; break;
                    case GL_INVALID_OPERATION: errorName = "GL_INVALID_OPERATION"; break;
                    case GL_INVALID_FRAMEBUFFER_OPERATION: errorName = "GL_INVALID_FRAMEBUFFER_OPERATION"; break;
                    case GL_OUT_OF_MEMORY: errorName = "GL_OUT_OF_MEMORY"; break;
                    }
                    static std::unordered_set<std::string> reportedFailures;
                    const std::string failureKey = operation + ":" + std::to_string(static_cast<unsigned int>(error)) + ":" + std::to_string(vao);
                    if (!reportedFailures.insert(failureKey).second) continue;

                    std::string details;
                    if (operation == "glDrawElements") {
                        GLint boundVao = 0;
                        GLint boundEbo = 0;
                        GLint vaoIsObject = GL_FALSE;
                        GLint indexBufferBytes = -1;
                        GLint indexBufferMapped = 0;
                        GLint currentProgram = 0;
                        GLint activeSamplesQuery = 0;
                        GLint currentProgramLinked = 0;
                        GLboolean transformFeedbackActive = GL_FALSE;
                        GLboolean transformFeedbackPaused = GL_FALSE;
                        glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &boundVao);
                        glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, &boundEbo);
                        vaoIsObject = glIsVertexArray(static_cast<GLuint>(boundVao)) == GL_TRUE;
                        if (boundEbo != 0) {
                            glGetBufferParameteriv(GL_ELEMENT_ARRAY_BUFFER, GL_BUFFER_SIZE, &indexBufferBytes);
                            glGetBufferParameteriv(GL_ELEMENT_ARRAY_BUFFER, GL_BUFFER_MAPPED, &indexBufferMapped);
                        }
                        glGetIntegerv(GL_CURRENT_PROGRAM, &currentProgram);
                        if (currentProgram != 0) {
                            glGetProgramiv(static_cast<GLuint>(currentProgram), GL_LINK_STATUS, &currentProgramLinked);
                        }
                        glGetQueryiv(GL_SAMPLES_PASSED, GL_CURRENT_QUERY, &activeSamplesQuery);
                        glGetBooleanv(GL_TRANSFORM_FEEDBACK_ACTIVE, &transformFeedbackActive);
                        glGetBooleanv(GL_TRANSFORM_FEEDBACK_PAUSED, &transformFeedbackPaused);
                        const unsigned int maxIndex = meshIndices.empty() ? 0 :
                            *std::max_element(meshIndices.begin(), meshIndices.end());

                        details = " expectedVAO=" + std::to_string(vao) +
                            ", boundVAO=" + std::to_string(boundVao) +
                            ", VAOisObject=" + std::to_string(vaoIsObject) +
                            ", expectedEBO=" + std::to_string(ebo) +
                            ", boundEBO=" + std::to_string(boundEbo) +
                            ", EBOisBuffer=" + std::to_string(glIsBuffer(static_cast<GLuint>(boundEbo)) == GL_TRUE) +
                            ", EBObytes=" + std::to_string(indexBufferBytes) +
                            ", requiredBytes=" + std::to_string(indexCount * sizeof(unsigned int)) +
                            ", EBOmapped=" + std::to_string(indexBufferMapped) +
                            ", currentProgram=" + std::to_string(currentProgram) +
                            ", programLinked=" + std::to_string(currentProgramLinked) +
                            ", activeSamplesQuery=" + std::to_string(activeSamplesQuery) +
                            ", transformFeedbackActive=" + std::to_string(transformFeedbackActive == GL_TRUE) +
                            ", transformFeedbackPaused=" + std::to_string(transformFeedbackPaused == GL_TRUE) +
                            ", vertices=" + std::to_string(vertexCount) +
                            ", maxIndex=" + std::to_string(maxIndex) +
                            ", indexRangeValid=" + std::to_string(meshIndices.empty() || maxIndex < vertexCount);

                        for (GLuint attribute = 0; attribute < 3; ++attribute) {
                            GLint enabled = 0;
                            GLint buffer = 0;
                            GLint size = 0;
                            GLint type = 0;
                            GLint stride = 0;
                            GLint bufferMapped = 0;
                            glGetVertexAttribiv(attribute, GL_VERTEX_ATTRIB_ARRAY_ENABLED, &enabled);
                            glGetVertexAttribiv(attribute, GL_VERTEX_ATTRIB_ARRAY_BUFFER_BINDING, &buffer);
                            glGetVertexAttribiv(attribute, GL_VERTEX_ATTRIB_ARRAY_SIZE, &size);
                            glGetVertexAttribiv(attribute, GL_VERTEX_ATTRIB_ARRAY_TYPE, &type);
                            glGetVertexAttribiv(attribute, GL_VERTEX_ATTRIB_ARRAY_STRIDE, &stride);
                            if (buffer != 0 && glIsBuffer(static_cast<GLuint>(buffer)) == GL_TRUE) {
                                GLint previousArrayBuffer = 0;
                                glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &previousArrayBuffer);
                                glBindBuffer(GL_ARRAY_BUFFER, static_cast<GLuint>(buffer));
                                glGetBufferParameteriv(GL_ARRAY_BUFFER, GL_BUFFER_MAPPED, &bufferMapped);
                                glBindBuffer(GL_ARRAY_BUFFER, static_cast<GLuint>(previousArrayBuffer));
                            }
                            details += ", attrib" + std::to_string(attribute) + "={enabled=" +
                                std::to_string(enabled) + ", buffer=" + std::to_string(buffer) +
                                ", bufferIsObject=" + std::to_string(buffer != 0 && glIsBuffer(static_cast<GLuint>(buffer)) == GL_TRUE) +
                                ", bufferMapped=" + std::to_string(bufferMapped) +
                                ", size=" + std::to_string(size) + ", type=" + std::to_string(type) +
                                ", stride=" + std::to_string(stride) + "}";
                        }
                    }

                    LOG_ERROR("[Mesh Diagnostics] " + operation + " failed: " + errorName +
                        " (" + std::to_string(static_cast<unsigned int>(error)) + "), VAO=" +
                        std::to_string(vao) + ", EBO=" + std::to_string(ebo) +
                        ", indices=" + std::to_string(indexCount) + details + ".");
                }
            }
        }

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
            const bool diagnose = Settings::getInstance().debugMode;
            unsigned int diffuseNr = 1;
            unsigned int specularNr = 1;
            unsigned int normalNr = 1;

            for (unsigned int i = 0; i < textures.size(); i++) {
                glActiveTexture(GL_TEXTURE0 + i);
                if (diagnose) CheckMeshOpenGLError("glActiveTexture(unit=" + std::to_string(i) + ")", VAO, EBO, indices, vertices.size());

                std::string number;
                const std::string& name = textures[i].type;

                if (name == "texture_diffuse")
                    number = std::to_string(diffuseNr++);
                else if (name == "texture_specular")
                    number = std::to_string(specularNr++);
                else if (name == "texture_normal")
                    number = std::to_string(normalNr++); // ��������� ��������� �������� � ������

                shader.setInt(("material." + name + number).c_str(), i);
                if (diagnose) CheckMeshOpenGLError("setting texture sampler for unit " + std::to_string(i), VAO, EBO, indices, vertices.size());
                glBindTexture(GL_TEXTURE_2D, textures[i].id);
                if (diagnose) CheckMeshOpenGLError("glBindTexture(unit=" + std::to_string(i) + ")", VAO, EBO, indices, vertices.size());
            }

            glBindVertexArray(VAO);
            if (diagnose) CheckMeshOpenGLError("glBindVertexArray", VAO, EBO, indices, vertices.size());
            glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(indices.size()), GL_UNSIGNED_INT, nullptr);
            if (diagnose) CheckMeshOpenGLError("glDrawElements", VAO, EBO, indices, vertices.size());
            glBindVertexArray(0);
            if (diagnose) CheckMeshOpenGLError("glBindVertexArray(0)", VAO, EBO, indices, vertices.size());

            glActiveTexture(GL_TEXTURE0);
            if (diagnose) CheckMeshOpenGLError("glActiveTexture(GL_TEXTURE0)", VAO, EBO, indices, vertices.size());
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