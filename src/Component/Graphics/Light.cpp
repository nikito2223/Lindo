#include "Light.h"
#include "Debug/DebugLogger.h"
#include <glm/gtc/matrix_transform.hpp>

namespace Lindo {
    namespace Components {
        namespace Light {

            // ==========================================
            // Реализация базового класса Light
            // ==========================================
            Light::Light(LightType t, const std::string& lightName)
                : type(t), name(lightName) {
            }

            glm::vec3 Light::getPosition() const {
                return owner ? owner->getWorldPosition() : glm::vec3(0.0f);
            }

            void Light::SetBaseColor(const glm::vec3& baseColor) {
                color = baseColor;
                ambient = baseColor * 0.1f;
                diffuse = baseColor * 0.8f;
                specular = glm::vec3(1.0f);
            }

            void Light::Toggle() {
                enabled = !enabled;
                LOG_INFO("Light '" + name + "' toggled. Enabled: " + std::string(enabled ? "true" : "false"));
            }

            // ==========================================
            // Реализация DirectionalLight
            // ==========================================
            DirectionalLight::DirectionalLight(const std::string& lightName)
                : Light(LightType::Directional, lightName) {
            }

            void DirectionalLight::ApplyToShader(Lindo::Graphics::Shader& shader, const std::string& uniformName) const {
                shader.setBool(uniformName + ".enabled", enabled);
                if (!enabled) return;

                glm::vec3 worldDirection = direction;
                if (owner) {
                    worldDirection = glm::normalize(glm::mat3(owner->getWorldMatrix()) * direction);
                }

                shader.setVec3(uniformName + ".direction", worldDirection);
                shader.setVec3(uniformName + ".ambient", ambient * intensity);
                shader.setVec3(uniformName + ".diffuse", diffuse * intensity);
                shader.setVec3(uniformName + ".specular", specular * intensity);
                shader.setVec3(uniformName + ".color", color);
            }

            // ==========================================
            // Реализация PointLight
            // ==========================================
            PointLight::PointLight(const std::string& lightName)
                : Light(LightType::Point, lightName) {
            }

            void PointLight::SetRadius(float newRadius) {
                radius = newRadius;
                constant = 1.0f;
                linear = 4.5f / radius;
                quadratic = 75.0f / (radius * radius);
            }

            void PointLight::ApplyToShader(Lindo::Graphics::Shader& shader, const std::string& uniformName) const {
                shader.setBool(uniformName + ".enabled", enabled);
                if (!enabled) return;

                shader.setVec3(uniformName + ".position", getPosition());
                shader.setFloat(uniformName + ".constant", constant);
                shader.setFloat(uniformName + ".linear", linear);
                shader.setFloat(uniformName + ".quadratic", quadratic);
                shader.setFloat(uniformName + ".radius", radius);

                shader.setVec3(uniformName + ".ambient", ambient * intensity);
                shader.setVec3(uniformName + ".diffuse", diffuse * intensity);
                shader.setVec3(uniformName + ".specular", specular * intensity);
                shader.setVec3(uniformName + ".color", color);
            }

            // ==========================================
            // Реализация SpotLight
            // ==========================================
            SpotLight::SpotLight(const std::string& lightName)
                : Light(LightType::Spot, lightName) {
            }

            void SpotLight::SetRadius(float newRadius) {
                radius = newRadius;
                constant = 1.0f;
                linear = 4.5f / radius;
                quadratic = 75.0f / (radius * radius);
            }

            void SpotLight::ApplyToShader(Lindo::Graphics::Shader& shader, const std::string& uniformName) const {
                shader.setBool(uniformName + ".enabled", enabled);
                if (!enabled) return;

                glm::vec3 worldDirection = direction;
                if (owner) {
                    worldDirection = glm::normalize(glm::mat3(owner->getWorldMatrix()) * direction);
                }

                shader.setVec3(uniformName + ".position", getPosition());
                shader.setVec3(uniformName + ".direction", worldDirection);
                shader.setFloat(uniformName + ".cutOff", cutOff);
                shader.setFloat(uniformName + ".outerCutOff", outerCutOff);

                shader.setFloat(uniformName + ".constant", constant);
                shader.setFloat(uniformName + ".linear", linear);
                shader.setFloat(uniformName + ".quadratic", quadratic);
                shader.setFloat(uniformName + ".radius", radius);

                shader.setVec3(uniformName + ".ambient", ambient * intensity);
                shader.setVec3(uniformName + ".diffuse", diffuse * intensity);
                shader.setVec3(uniformName + ".specular", specular * intensity);
                shader.setVec3(uniformName + ".color", color);
            }

        }
    }
}
