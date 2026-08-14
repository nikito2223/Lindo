#pragma once

#include <Graphics/core/Shader.h>
#include <Component/Component.h>
#include <Component/GameObject/GameObject.h>
#include <glm/glm.hpp>
#include <string>
#include <vector>
#include <memory>

namespace Lindo {
    namespace Components {
        namespace Light {

            enum class LightType {
                Directional,
                Point,
                Spot
            };

            using GameObjectList = std::vector<std::unique_ptr<Lindo::World::GameObject>>;

            class Light : public Lindo::World::Component {
            public:
                Light(LightType t, const std::string& lightName = "Light");
                virtual ~Light() = default;

                LightType type;
                std::string name;
                bool enabled = true;

                glm::vec3 color = glm::vec3(1.0f);
                float intensity = 1.0f;

                glm::vec3 ambient = glm::vec3(0.1f);
                glm::vec3 diffuse = glm::vec3(0.8f);
                glm::vec3 specular = glm::vec3(1.0f);

                bool castShadows = true;
                float farPlane = 25.0f;

                glm::vec3 getPosition() const;
                void SetBaseColor(const glm::vec3& baseColor);
                void Toggle();

                virtual void ApplyToShader(Lindo::Graphics::Shader& shader, const std::string& uniformName) const = 0;
            };

            class DirectionalLight : public Light {
            public:
                DirectionalLight(const std::string& lightName = "DirectionalLight");

                glm::vec3 direction = glm::vec3(0.0f, -1.0f, 0.0f);

                void ApplyToShader(Lindo::Graphics::Shader& shader, const std::string& uniformName) const override;
            };

            class PointLight : public Light {
            public:
                PointLight(const std::string& lightName = "PointLight");

                float radius = 10.0f;
                float constant = 1.0f;
                float linear = 0.09f;
                float quadratic = 0.032f;

                void SetRadius(float newRadius);
                void ApplyToShader(Lindo::Graphics::Shader& shader, const std::string& uniformName) const override;
            };

            class SpotLight : public Light {
            public:
                SpotLight(const std::string& lightName = "SpotLight");

                glm::vec3 direction = glm::vec3(0.0f, -1.0f, 0.0f);
                float radius = 15.0f;
                float constant = 1.0f;
                float linear = 0.09f;
                float quadratic = 0.032f;

                float cutOff = glm::cos(glm::radians(12.5f));
                float outerCutOff = glm::cos(glm::radians(17.5f));

                void SetRadius(float newRadius);
                void ApplyToShader(Lindo::Graphics::Shader& shader, const std::string& uniformName) const override;
            };

        }
    }
}