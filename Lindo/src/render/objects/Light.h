#pragma once
#include "Object.h"
#include <glm/glm.hpp>

enum LightType { DIRECTIONAL, POINT, SPOT };

class Light : public Object {
public:
    LightType type;

    // Основные параметры (работают с твоим шейдером)
    glm::vec3 color = glm::vec3(1.0f);      // Основной цвет света
    float intensity = 1.0f;                  // Интенсивность света

    // Компоненты освещения (используются в шейдере)
    glm::vec3 ambient = glm::vec3(0.1f);
    glm::vec3 diffuse = glm::vec3(0.8f);
    glm::vec3 specular = glm::vec3(1.0f);

    // Коэффициенты затухания
    float constant = 1.0f;
    float linear = 0.09f;
    float quadratic = 0.032f;

    bool castShadows = true;          // Отбрасывает ли тени
    float shadowBias = 0.005f;        // Смещение для избежания shadow acne
    float shadowSoftness = 1.0f;      // Мягкость теней
    glm::vec3 shadowColor = glm::vec3(0.0f, 0.0f, 0.0f); // Цвет теней

    float farPlane = 25.0f;           // Дальняя плоскость для теней

// Для прожекторов
    float shadowAngle = 45.0f;        // Угол тени прожектора

    // Для направленного света и прожекторов
    glm::vec3 direction = glm::vec3(0.0f, -1.0f, 0.0f);

    // Для прожектора
    float cutOff = glm::cos(glm::radians(12.5f));
    float outerCutOff = glm::cos(glm::radians(17.5f));

    // 🔧 ДОБАВЛЕНО: Позиция для быстрого доступа
    glm::vec3& position = transform.position;

    // Управление
    bool enabled = true;
    bool showDebugIcon = true;

    // 🔧 ДОБАВЛЕНО: Имя для идентификации
    std::string name = "Light";

    // Для debug визуализации
    glm::vec3 debugColor;
    Mesh* debugMesh = nullptr;

    Light(LightType t, const std::string& lightName = "Light")
        : type(t), name(lightName)
    {
        // Устанавливаем debug цвет в зависимости от типа
        switch (type) {
        case DIRECTIONAL:
            debugColor = glm::vec3(1.0f, 1.0f, 0.0f); // Желтый
            break;
        case POINT:
            debugColor = glm::vec3(1.0f, 0.5f, 0.0f); // Оранжевый
            break;
        case SPOT:
            debugColor = glm::vec3(0.0f, 1.0f, 1.0f); // Голубой
            break;
        }

        // Автоматически создаем debug меш
        createDebugMesh();
    }

    ~Light() {
        if (debugMesh) {
            delete debugMesh;
        }
    }

    // 🔧 ДОБАВЛЕНО: Метод для быстрого включения/выключения
    void Toggle() {
        enabled = !enabled;
        std::cout << "Light '" << name << "' is now "
            << (enabled ? "ON" : "OFF") << std::endl;
    }

    // 🔧 ДОБАВЛЕНО: Установка позиции с выводом
    void SetPosition(const glm::vec3& newPos) {
        position = newPos;
        std::cout << "Light '" << name << "' position: "
            << position.x << ", " << position.y << ", " << position.z << std::endl;
    }

    // 🔧 ДОБАВЛЕНО: Установка параметров с выводом
    void SetParameters(const glm::vec3& col, float intens, float radius = 10.0f) {
        color = col;
        intensity = intens;
        SetRadius(radius);

        std::cout << "Light '" << name << "' parameters:" << std::endl;
        std::cout << "  Color: " << color.r << ", " << color.g << ", " << color.b << std::endl;
        std::cout << "  Intensity: " << intensity << std::endl;
        std::cout << "  Radius: " << GetRadius() << std::endl;
    }

    // 🔧 ДОБАВЛЕНО: Получение радиуса влияния
    float GetRadius() const {
        float maxLight = 1.0f / (constant + linear * 0.05f + quadratic * 0.0025f);
        float targetIntensity = 0.05f * maxLight;

        float a = quadratic * targetIntensity;
        float b = linear * targetIntensity;
        float c = constant * targetIntensity - 1.0f;

        float discriminant = b * b - 4.0f * a * c;
        if (discriminant < 0) return 10.0f;

        return static_cast<float>((-b + std::sqrt(discriminant)) / (2.0 * a));
    }

    // Метод для установки базового цвета (также устанавливает ambient/diffuse/specular)
    void SetBaseColor(const glm::vec3& baseColor) {
        color = baseColor;
        // Автоматически настраиваем компоненты на основе базового цвета
        ambient = baseColor * 0.1f;
        diffuse = baseColor * 0.8f;
        specular = baseColor * 1.0f;
    }

    // Метод для настройки радиуса влияния (автоматически рассчитывает коэффициенты)
    void SetRadius(float radius) {
        if (radius > 0) {
            // Автоматический расчет коэффициентов затухания на основе радиуса
            // Используем формулу для затухания до ~5% на границе радиуса
            constant = 1.0f;
            linear = 4.5f / radius;
            quadratic = 75.0f / (radius * radius);
        }
    }

    // Получение финального цвета с учетом интенсивности
    glm::vec3 GetFinalColor() const {
        return color * intensity;
    }

    // Расчет влияния света на точку (для оптимизации)
    float CalculateInfluence(const glm::vec3& pointPos) const {
        if (!enabled) return 0.0f;

        float distance = glm::length(pointPos - position);

        // Для направленного света всегда максимальное влияние
        if (type == DIRECTIONAL) return 1.0f;

        // Для точечного и прожектора - затухание
        float attenuation = 1.0f / (constant + linear * distance + quadratic * (distance * distance));
        return attenuation;
    }

    // 🔧 ДОБАВЛЕНО: Метод для получения информации о свете
    std::string GetInfo() const {
        std::string info = "[" + name + "] ";
        info += "Type: ";
        switch (type) {
        case DIRECTIONAL: info += "Directional"; break;
        case POINT: info += "Point"; break;
        case SPOT: info += "Spot"; break;
        }
        info += " | Position: (" + std::to_string(position.x) + ", "
            + std::to_string(position.y) + ", "
            + std::to_string(position.z) + ")";
        info += " | " + std::string(enabled ? "ON" : "OFF");
        info += " | Color: (" + std::to_string(color.r) + ", "
            + std::to_string(color.g) + ", "
            + std::to_string(color.b) + ")";
        info += " | Intensity: " + std::to_string(intensity);
        return info;
    }

    // Метод для debug отрисовки
    void DrawDebug(Shader& debugShader) {
        if (!showDebugIcon || !debugMesh || !enabled) return;

        // Устанавливаем модельную матрицу
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, position);

        // Масштабируем в зависимости от типа света
        float scale = 0.3f * sqrt(intensity); // Масштаб зависит от интенсивности
        model = glm::scale(model, glm::vec3(scale));

        // 🔧 ДОБАВЛЕНО: Анимация пульсации для включенного источника
        if (enabled) {
            double pulse = sin(glfwGetTime() * 2.0f) * 0.1f + 1.0f;
            model = glm::scale(model, glm::vec3(pulse));
        }

        debugShader.setMat4("model", model);

        // 🔧 ИЗМЕНЕНО: Используем цвет света для debug визуализации
        glm::vec3 drawColor = enabled ? color * intensity : glm::vec3(0.3f);
        debugShader.setVec3("color", drawColor);

        // Для шейдера, который ожидает alpha (если нужно)
        debugShader.setFloat("alpha", enabled ? 0.9f : 0.3f);

        debugMesh->Draw(debugShader);
    }

    // 🔧 ДОБАВЛЕНО: Упрощенный debug draw (без анимации)
    void DrawSimpleDebug(Shader& debugShader) {
        if (!debugMesh) return;

        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, position);
        model = glm::scale(model, glm::vec3(0.2f));

        debugShader.setMat4("model", model);
        debugShader.setVec3("color", enabled ? debugColor : glm::vec3(0.3f));

        debugMesh->Draw(debugShader);
    }

    // Основной метод для применения света к шейдеру
    void ApplyToShader(Shader& shader, const std::string& name)
    {
        shader.setBool(name + ".enabled", enabled);

        if (!enabled) {
            // гарантируем, что вклад = 0
            shader.setVec3(name + ".ambient", glm::vec3(0.0f));
            shader.setVec3(name + ".diffuse", glm::vec3(0.0f));
            shader.setVec3(name + ".specular", glm::vec3(0.0f));
            return;
        }

        shader.setVec3(name + ".color", color);
        shader.setFloat(name + ".intensity", intensity);

        shader.setVec3(name + ".ambient", ambient * intensity);
        shader.setVec3(name + ".diffuse", diffuse * intensity);
        shader.setVec3(name + ".specular", specular * intensity);

        if (type == POINT || type == SPOT) {
            shader.setVec3(name + ".position", position);
            shader.setFloat(name + ".constant", constant);
            shader.setFloat(name + ".linear", linear);
            shader.setFloat(name + ".quadratic", quadratic);
        }

        if (type == DIRECTIONAL || type == SPOT) {
            shader.setVec3(name + ".direction", direction);
        }

        if (type == SPOT) {
            shader.setFloat(name + ".cutOff", cutOff);
            shader.setFloat(name + ".outerCutOff", outerCutOff);
        }
    }

    // Перегрузка для работы с массивами источников
    void ApplyToShader(Shader& shader, const std::string& name, int index) {
        std::string lightName = name + "[" + std::to_string(index) + "]";
        ApplyToShader(shader, lightName);
    }

private:
    void createDebugMesh() {
        // Создаем сферу для визуализации точечного источника
        std::vector<Vertex> vertices;
        std::vector<unsigned int> indices;

        // Создание UV-сферы
        const int segments = 16;
        const int rings = 16;
        const float radius = 0.2f;

        for (int i = 0; i <= rings; ++i) {
            float phi = glm::pi<float>() * (float)i / (float)rings;

            for (int j = 0; j <= segments; ++j) {
                float theta = 2.0f * glm::pi<float>() * (float)j / (float)segments;

                float x = radius * sin(phi) * cos(theta);
                float y = radius * cos(phi);
                float z = radius * sin(phi) * sin(theta);

                glm::vec3 normal = glm::normalize(glm::vec3(x, y, z));
                glm::vec2 uv = glm::vec2((float)j / segments, (float)i / rings);

                vertices.push_back({
                    glm::vec3(x, y, z),
                    normal,
                    uv
                    });
            }
        }

        for (int i = 0; i < rings; ++i) {
            for (int j = 0; j < segments; ++j) {
                int first = (i * (segments + 1)) + j;
                int second = first + segments + 1;

                indices.push_back(first);
                indices.push_back(second);
                indices.push_back(first + 1);

                indices.push_back(first + 1);
                indices.push_back(second);
                indices.push_back(second + 1);
            }
        }

        debugMesh = new Mesh(vertices, indices, std::vector<Texture>());
    }
};