#pragma once
#include "Graphics/core/ShadowMap.h"
#include <glm/glm.hpp>
#include <memory>
#include <iostream>
#include <string>
#include <vector>
#include <GLFW/glfw3.h>  // для glfwGetTime (пульсация иконки)
#include <Component/Component.h>
#include <Component/GameObject/GameObject.h>  // Добавить!
#include <Graphics/core/mesh.h>

// Типы источников светаs
enum class LightType
{
    Directional, // Направленный (бесконечно удалённый, параллельные лучи)
    Point,       // Точечный (излучает во все стороны из позиции)
    Spot         // Прожектор (конус света)
};

// Класс источника света, наследует Object (имеет трансформацию)
class Light : public Component {
public:
    Light() = default;

    LightType type;                     // Тип источника

    // ----- Основные световые параметры (передаются в шейдер) -----
    glm::vec3 color = glm::vec3(1.0f);  // Цвет света (RGB)
    float intensity = 1.0f;  // Множитель яркости
    float radius = 10.0f;    // Эффективный радиус света

    // Компоненты освещения по Фонгу (ambient, diffuse, specular)
    glm::vec3 ambient = glm::vec3(0.1f);
    glm::vec3 diffuse = glm::vec3(0.8f);
    glm::vec3 specular = glm::vec3(1.0f);

    // Коэффициенты затухания для точечных источников и прожекторов
    // Аттенюация = 1 / (constant + linear * dist + quadratic * dist^2)
    float constant = 1.0f;
    float linear = 0.09f;
    float quadratic = 0.032f;

    // ----- Параметры теней -----
    bool castShadows = true;             // Включено ли отбрасывание теней
    float shadowBias = 0.005f;           // Смещение глубины для борьбы с shadow acne
    float shadowSoftness = 1.0f;         // Коэффициент мягкости (PCF и т.п.)
    glm::vec3 shadowColor = glm::vec3(0.0f, 0.0f, 0.0f); // Цвет затенённых областей
    float farPlane = 25.0f;              // Дальняя плоскость отсечения для теневых карт
    glm::mat4 lightSpaceMatrix;
    // Угол тени прожектора (для настройки карты теней, если нужно)
    float shadowAngle = 45.0f;

    // ----- Геометрические параметры -----
    glm::vec3 direction = glm::vec3(0.0f, -1.0f, 0.0f); // Направление (для Dir и Spot)

    // Углы конуса прожектора (хранятся как косинусы для шейдера)
    float cutOff = glm::cos(glm::radians(12.5f));       // внутренний угол
    float outerCutOff = glm::cos(glm::radians(17.5f));  // внешний угол

    // ----- Управление и идентификация -----
    bool enabled = true;                  // Включён ли свет
    bool showDebugIcon = true;             // Показывать ли иконку в редакторе/отладке
    std::string name = "Light";            // Имя источника

    // ----- Ресурсы для теней -----
    std::unique_ptr<ShadowMap> shadowMap;          // Теневая карта (FBO + текстура глубины)
    std::unique_ptr<Shader> shadowDepthShader;     // Шейдер для рендера глубины
    int shadowMapWidth = 1024;                      // Ширина карты теней
    int shadowMapHeight = 1024;                      // Высота карты тенейы
    
    glm::vec3 getPosition() const {
        return owner ? owner->transform.position : glm::vec3(0.0f);
    }

    // ----- Конструктор -----
    Light(LightType t, const std::string& lightName = "Light", bool autoInitShadows = true)
        : Component(),  // ✅ Явный вызов конструктора Component
        type(t),
        name(lightName)
    {
        // Выбор цвета иконки
        //switch (type) {
        //case LightType::Directional:
        //    debugColor = glm::vec3(1.0f, 1.0f, 0.0f);
        //    break;
        //case LightType::Point:
        //    debugColor = glm::vec3(1.0f, 0.5f, 0.0f);
        //    break;
        //case LightType::Spot:
        //    debugColor = glm::vec3(0.0f, 1.0f, 1.0f);
        //    break;
        //}

        // Инициализация теней
        if (castShadows && autoInitShadows) {
            glCullFace(GL_FRONT);
            InitShadowResources(shadowMapWidth, shadowMapHeight);
            glCullFace(GL_BACK);
        }
    }

    // ----- Деструктор -----
    ~Light() {

    }

    // Инициализация карты теней и шейдера глубины
    void InitShadowResources(int width = 2048, int height = 2048) {
        shadowMapWidth = width;
        shadowMapHeight = height;

        shadowMap = std::make_unique<ShadowMap>();
        if (type == LightType::Point)
            shadowMap->Init(width, height, ShadowType::PointCube);
        else
            shadowMap->Init(width, height, ShadowType::Directional2D);

        // Загрузка шейдера для рендера глубины (пути заданы в глобальной переменной PathData)
        shadowDepthShader = std::make_unique<Shader>(
            (PathData + "shaders/Light/shadow_depth.vs").c_str(),
            (PathData + "shaders/Light/shadow_depth.fs").c_str()
            );
    }

    // Перегруженная версия (может учитывать список объектов для оптимальной ортографической проекции)
    void ComputeLightMatrices(glm::mat4& proj, glm::mat4& view) const;

    // ----- Методы управления состоянием -----

    // Включить/выключить свет
    void Toggle() {
        enabled = !enabled;
        std::cout << "Light '" << name << "' is now "
            << (enabled ? "ON" : "OFF") << std::endl;
    }

    // Установить цвет, интенсивность и радиус действия (для точечных и прожекторов)
    void SetParameters(const glm::vec3& col, float intens, float radius = 10.0f) {
        color = col;
        intensity = intens;
        SetRadius(radius);

        std::cout << "Light '" << name << "' parameters:" << std::endl;
        std::cout << "  Color: " << color.r << ", " << color.g << ", " << color.b << std::endl;
        std::cout << "  Intensity: " << intensity << std::endl;
        std::cout << "  Radius: " << GetRadius() << std::endl;
    }

    // Рассчитать эффективный радиус действия света на основе затухания
    float GetRadius() const {
        return radius;
    }

    // Установить базовый цвет и автоматически настроить ambient/diffuse/specular
    void SetBaseColor(const glm::vec3& baseColor) {
        color = baseColor;
        ambient = baseColor * 0.1f;
        diffuse = baseColor * 0.8f;
        specular = glm::vec3(1.0f);
    }

    // Установить радиус действия, пересчитав коэффициенты затухания
    void SetRadius(float newRadius);

    // Получить итоговый цвет с учётом интенсивности (color * intensity)
    glm::vec3 GetFinalColor() const {
        return color * intensity;
    }

    // Вычислить влияние света в заданной точке (аттенюация для Point/Spot, 1.0 для Directional)
    float CalculateInfluence(const glm::vec3& pointPos) const {
        if (!enabled) return 0.0f;

        if (type == LightType::Directional) return intensity; // бесконечно дальний

        float distance = glm::length(pointPos - getPosition());
        float attenuation = 1.0f / (constant + linear * distance + quadratic * distance * distance);
        return attenuation * intensity;
    }

    // ---- Отладочная иконка ----
    float GetDebugScale() const {
        return 0.2f + 0.3f * radius * sqrt(intensity); // масштабируем визуально
    }

    // Получить строку с информацией о свете (для отладки)
    std::string GetInfo() const {
        std::string info = "[" + name + "] ";
        info += "Type: ";
        switch (type) {
        case LightType::Directional: info += "Directional"; break;
        case LightType::Point: info += "Point"; break;
        case LightType::Spot: info += "Spot"; break;
        }
        info += " | Position: (" + std::to_string(getPosition().x) + ", "
            + std::to_string(getPosition().y) + ", "
            + std::to_string(getPosition().z) + ")";
        info += " | " + std::string(enabled ? "ON" : "OFF");
        info += " | Color: (" + std::to_string(color.r) + ", "
            + std::to_string(color.g) + ", "
            + std::to_string(color.b) + ")";
        info += " | Intensity: " + std::to_string(intensity);
        return info;
    }

    // ----- Методы отрисовки отладочной иконки -----

    // Простая отрисовка (без эффектов)
    //void DrawSimpleDebug(Shader& debugShader) {
    //    if (!debugMesh) return;

    //    glm::mat4 model = glm::mat4(1.0f);
    //    model = glm::translate(model, getPosition());
    //    model = glm::scale(model, glm::vec3(0.2f));

    //    debugShader.setMat4("model", model);
    //    debugShader.setVec3("color", enabled ? debugColor : glm::vec3(0.3f));

    //    debugMesh->Draw(debugShader);
    //}

    // ----- Передача параметров света в шейдер -----
    void ComputeLightMatrices(glm::mat4& proj, glm::mat4& view, const std::vector<GameObject*>& objects) const;

    void ComputePointLightMatrices(std::vector<glm::mat4>& shadowTransforms) const
    {
        shadowTransforms.clear();

        glm::mat4 shadowProj = glm::perspective(
            glm::radians(90.0f),
            1.0f,
            1.0f,
            farPlane
        );

        shadowTransforms.push_back(
            shadowProj * glm::lookAt(getPosition(),
                getPosition() + glm::vec3(1, 0, 0),
                glm::vec3(0, -1, 0))
        );

        shadowTransforms.push_back(
            shadowProj * glm::lookAt(getPosition(),
                getPosition() + glm::vec3(-1, 0, 0),
                glm::vec3(0, -1, 0))
        );

        shadowTransforms.push_back(
            shadowProj * glm::lookAt(getPosition(),
                getPosition() + glm::vec3(0, 1, 0),
                glm::vec3(0, 0, 1))
        );

        shadowTransforms.push_back(
            shadowProj * glm::lookAt(getPosition(),
                getPosition() + glm::vec3(0, -1, 0),
                glm::vec3(0, 0, -1))
        );

        shadowTransforms.push_back(
            shadowProj * glm::lookAt(getPosition(),
                getPosition() + glm::vec3(0, 0, 1),
                glm::vec3(0, -1, 0))
        );
        
        shadowTransforms.push_back(
            shadowProj * glm::lookAt(getPosition(),
                getPosition() + glm::vec3(0, 0, -1),
                glm::vec3(0, -1, 0))
        );
    }

    // Установить uniform-ы для одного источника с заданным именем
    void ApplyToShader(Shader& shader, const std::string& name)
    {
        shader.setBool(name + ".enabled", enabled);

        if (!enabled) {
            shader.setVec3(name + ".ambient", glm::vec3(0.0f));
            shader.setVec3(name + ".diffuse", glm::vec3(0.0f));
            shader.setVec3(name + ".specular", glm::vec3(0.0f));
            return;
        }

        shader.setVec3(name + ".color", color);
        shader.setFloat(name + ".intensity", intensity);

        shader.setVec3(name + ".ambient", ambient);
        shader.setVec3(name + ".diffuse", diffuse);
        shader.setVec3(name + ".specular", specular);

        shader.setBool("shadowsEnabled", true);

        if (type == LightType::Point || type == LightType::Spot) {
            shader.setVec3(name + ".position", getPosition());
            shader.setFloat(name + ".constant", constant);
            shader.setFloat(name + ".linear", linear);
            shader.setFloat(name + ".quadratic", quadratic);

            // Добавьте эту строку для передачи радиуса
            shader.setFloat(name + ".radius", radius);

        }

        if (type == LightType::Directional || type == LightType::Spot) {
            shader.setVec3(name + ".direction", direction);
        }
        
        if (type == LightType::Spot) {
            shader.setFloat(name + ".cutOff", cutOff);
            shader.setFloat(name + ".outerCutOff", outerCutOff);
        }
    }

    // Версия для массива источников (индекс в массиве)
    void ApplyToShader(Shader& shader, const std::string& name, int index) {
        std::string lightName = name + "[" + std::to_string(index) + "]";
        ApplyToShader(shader, lightName);
    }

    // Заглушки для методов, которые должны быть реализованы в .cpp (рендер теней, отрисовка радиуса)
    void RenderShadows(const std::vector<GameObject*>& objects, int screenWidth, int screenHeight);
    void DrawRadius(Shader& shader) const;

    glm::mat4 GetLightSpaceMatrix() const
    {
        return lightSpaceMatrix;
    }

private:
    
};