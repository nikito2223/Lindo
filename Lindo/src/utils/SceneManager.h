#pragma once
#include <glm/glm.hpp>

class Player;
class Input;
class Shader;
class Camera;

class SceneManager {
public:
    SceneManager();
    ~SceneManager();

    void init();
    void update(float deltaTime, Input* input);
    void render(Shader& lightingShader, float deltaTime, bool debugMode, bool showLightIcons, float lightIconRadius);
    void cleanup();

    Player* getPlayer() const;
    Camera& getCamera() const;
    glm::vec3 getCharacterPosition() const;

private:
    // Можно хранить указатель на сцену, если она будет объектом
    // Но пока используем глобальные функции из Scene.h
};