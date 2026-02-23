#pragma once
#include <glm/glm.hpp>
#include <string>
#include <vector>
#include "shader/shader_s.h"

class Shader;
struct Texture;

class Material {
public:
    // Флаги
    bool useDiffuseTexture = true;
    bool useSpecularTexture = false;

    // Цвета материала (используются, если текстуры отключены)
    glm::vec3 diffuseColor = glm::vec3(0.8f, 0.8f, 0.8f);
    glm::vec3 specularColor = glm::vec3(1.0f, 1.0f, 1.0f);

    // Текстуры
    Texture* diffuseTexture = nullptr;
    Texture* specularTexture = nullptr;

    // Параметры
    float shininess = 32.0f;
    float roughness = 0.5f;
    float metallic = 0.0f;

    // Метод применения материала к шейдеру
    void ApplyToShader(Shader& shader) {
        shader.setBool("material.useTexture", useDiffuseTexture);
        shader.setVec3("material.diffuseColor", diffuseColor);
        shader.setVec3("material.specularColor", specularColor);
        shader.setFloat("material.shininess", shininess);

        //// Привязка текстур
        //if (useDiffuseTexture && diffuseTexture) {
        //    diffuseTexture->Bind(0);
        //    shader.setInt("material.diffuse", 0);
        //}

        //if (useSpecularTexture && specularTexture) {
        //    specularTexture->Bind(1);
        //    shader.setInt("material.specular", 1);
        //}
    }

    // Установка цвета (автоматически отключает текстуры)
    void SetColor(const glm::vec3& color) {
        diffuseColor = color;
        specularColor = glm::vec3(1.0f); // Белые блики по умолчанию
        useDiffuseTexture = false;
        useSpecularTexture = false;
    }

    // Установка текстуры (автоматически включает использование текстур)
    void SetDiffuseTexture(Texture* texture) {
        diffuseTexture = texture;
        useDiffuseTexture = true;
    }

    void SetSpecularTexture(Texture* texture) {
        specularTexture = texture;
        useSpecularTexture = true;
    }
};