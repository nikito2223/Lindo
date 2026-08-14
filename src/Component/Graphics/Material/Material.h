#pragma once

#include <Component\Component.h>
#include <glm\fwd.hpp>

namespace Lindo {
	namespace Graphics {
		class Material : public Lindo::World::Component {
        public:
            unsigned int diffuseMap;
            unsigned int specularMap;
            float shininess;
            glm::vec3 color;
            bool useTexture; // <-- ƒќЅј¬»“№
            bool twoSided;

            Material(unsigned int diffuse, unsigned int specular, float shiny = 32.0f, bool doubleSided = false)
                : diffuseMap(diffuse), specularMap(specular), shininess(shiny),
                color(1.0f), twoSided(doubleSided), useTexture(true) { // ѕо умолчанию true
            }

            void apply(Shader& shader) {
                // 1. ќб€зательно передаем флаг в шейдер
                shader.setBool("material.useTexture", useTexture);
                shader.setVec3("material.color", color);
                shader.setFloat("material.shininess", shininess);

                if (useTexture && diffuseMap != 0) {
                    glActiveTexture(GL_TEXTURE0);
                    glBindTexture(GL_TEXTURE_2D, diffuseMap);
                    shader.setInt("material.diffuse", 0);
                }
                else {
                    // ≈сли текстуры нет, можно либо ничего не биндить, 
                    // либо забиндить 0, Ќќ тогда в шейдере ќЅя«ј“≈Ћ№Ќќ 
                    // должно быть условие if(useTexture)
                    glActiveTexture(GL_TEXTURE0);
                    glBindTexture(GL_TEXTURE_2D, 0);
                }

                if (useTexture && specularMap != 0) {
                    glActiveTexture(GL_TEXTURE1);
                    glBindTexture(GL_TEXTURE_2D, specularMap);
                    shader.setInt("material.specular", 1);
                }
                else {
                    glActiveTexture(GL_TEXTURE1);
                    glBindTexture(GL_TEXTURE_2D, 0);
                }

                // 2. —брос состо€ни€ отсечени€ граней
                if (twoSided) {
                    glDisable(GL_CULL_FACE);
                }
                else {
                    glEnable(GL_CULL_FACE);
                    glCullFace(GL_BACK);
                }
            }
		};
	}
}
