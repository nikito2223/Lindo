#pragma once
#include <Window/Camera.h>

class SceneManager;
class UIManager;
class DebugOverlay;
class Shader;
class Skybox;

class Renderer {
public:
    Renderer(SceneManager* scene, UIManager* ui, DebugOverlay* debug);
    ~Renderer();

    void init();   // загрузка шейдеров и skybox
    void render(float deltaTime);
    void onResize(int width, int height);

private:
    SceneManager* m_sceneManager;
    UIManager* m_uiManager;
    DebugOverlay* m_debugOverlay;

    Shader* m_lightingShader = nullptr;
    Shader* m_debugShader = nullptr;
    Shader* m_skyboxShader = nullptr;
    Skybox* m_skybox = nullptr;

    // Параметры рендера (можно вынести в настройки)
    bool m_showLightIcons = true;
    float m_lightIconRadius = 0.3f;
    int m_width = 0, m_height = 0;
    std::shared_ptr<Camera> m_camera; // предположительно
};