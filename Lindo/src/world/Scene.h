// Scene.h
#pragma once
#include <vector>
#include <memory>
#include <Objects/GameObject/Object.h>
#include <Objects/GameObject/light.h>
#include <Physics/PhysicsSystem.h>
#include <Physics/Collider/CollisionSystem.h>
#include <Window/Camera.h>
#include <Objects/Player.h>

// Функции сцены
void initScene();
void renderScene(Shader& shader, float deltaTime, Shader* debugShader = nullptr);
void cleanupScene();

Camera* getActiveCamera();
glm::vec3 getCharacterPosition();
Player* getPlayer();

// Настройка освещения
void setDirectionalLight(const glm::vec3& direction, const glm::vec3& ambient,
    const glm::vec3& diffuse, const glm::vec3& specular);
void setPointLight(int index, const glm::vec3& position, const glm::vec3& ambient,
    const glm::vec3& diffuse, const glm::vec3& specular,
    float constant = 1.0f, float linear = 0.09f, float quadratic = 0.032f);
void setSpotLight(const glm::vec3& position, const glm::vec3& direction,
    float cutOff, float outerCutOff, const glm::vec3& ambient,
    const glm::vec3& diffuse, const glm::vec3& specular);

// Геттеры/сеттеры для источников света
glm::vec3 getPointLightPosition(int index);
void setPointLightPosition(int index, const glm::vec3& position);
void updateSpotLightWithCamera();

// Получение всех источников света
std::vector<Light*>& getLights();