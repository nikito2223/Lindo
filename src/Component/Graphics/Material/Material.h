#pragma once

#include <Component\Component.h>
#include <glm\fwd.hpp>

namespace Lindo {
    namespace Graphics {
        class Material : public Lindo::World::Component {
        public:
            unsigned int diffuseMap  = 0;
            unsigned int specularMap = 0;
            float        shininess    = 32.0f;
            glm::vec3    color        = glm::vec3(1.0f);
            bool         twoSided     = false;

            // --- Unity-style UV transform ---
            glm::vec2 diffuseTiling  = glm::vec2(1.0f, 1.0f);
            glm::vec2 diffuseOffset  = glm::vec2(0.0f, 0.0f);
            glm::vec2 specularTiling = glm::vec2(1.0f, 1.0f);
            glm::vec2 specularOffset = glm::vec2(0.0f, 0.0f);

            Material(unsigned int diffuse = 0, unsigned int specular = 0,
                     float shiny = 32.0f, bool doubleSided = false)
                : diffuseMap(diffuse), specularMap(specular), shininess(shiny),
                  color(1.0f), twoSided(doubleSided) {
            }

            bool hasTexture() const { return diffuseMap != 0; }

            void setDiffuseTexture(unsigned int tex)  { diffuseMap  = tex; }
            void setSpecularTexture(unsigned int tex) { specularMap = tex; }
            void setColor(const glm::vec3& c)         { color = c; }
            void clearTexture() { diffuseMap = 0; specularMap = 0; }

            // --- UV: diffuse ---
            void setDiffuseTiling(const glm::vec2& t) { diffuseTiling = t; }
            void setDiffuseTiling(float x, float y)   { diffuseTiling = glm::vec2(x, y); }
            void setDiffuseOffset(const glm::vec2& o) { diffuseOffset = o; }
            void setDiffuseOffset(float x, float y)   { diffuseOffset = glm::vec2(x, y); }

            // --- UV: specular ---
            void setSpecularTiling(const glm::vec2& t) { specularTiling = t; }
            void setSpecularTiling(float x, float y)   { specularTiling = glm::vec2(x, y); }
            void setSpecularOffset(const glm::vec2& o) { specularOffset = o; }
            void setSpecularOffset(float x, float y)   { specularOffset = glm::vec2(x, y); }

            // Unity-подобный комбинированный сеттер (tiling + offset за раз)
            void setDiffuseUV(float tx, float ty, float ox, float oy) {
                diffuseTiling = glm::vec2(tx, ty);
                diffuseOffset = glm::vec2(ox, oy);
            }
            void setSpecularUV(float tx, float ty, float ox, float oy) {
                specularTiling = glm::vec2(tx, ty);
                specularOffset = glm::vec2(ox, oy);
            }

            void apply(Shader& shader) {
                shader.use();

                const bool hasDiffuse  = (diffuseMap  != 0);
                const bool hasSpecular = (specularMap != 0);

                shader.setBool ("material.useTexture", hasDiffuse);
                shader.setVec3 ("material.color",      color);
                shader.setFloat("material.shininess",  shininess);

                // --- UV-трансформы ---
                shader.setVec2("material.diffuseTiling",  diffuseTiling);
                shader.setVec2("material.diffuseOffset",  diffuseOffset);
                shader.setVec2("material.specularTiling", specularTiling);
                shader.setVec2("material.specularOffset", specularOffset);

                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, hasDiffuse ? diffuseMap : 0);
                shader.setInt("material.diffuse", 0);

                glActiveTexture(GL_TEXTURE1);
                glBindTexture(GL_TEXTURE_2D, hasSpecular ? specularMap
                                                         : (hasDiffuse ? diffuseMap : 0));
                shader.setInt("material.specular", 1);

                if (twoSided) {
                    glDisable(GL_CULL_FACE);
                } else {
                    glEnable(GL_CULL_FACE);
                    glCullFace(GL_BACK);
                }
            }
        };
    }
}