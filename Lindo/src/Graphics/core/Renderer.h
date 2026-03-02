#pragma once

class SceneManager;
class UIManager;
class PostProcessor;
class DebugOverlay;
class Shader;
class Skybox;

class Renderer {
public:
    Renderer(SceneManager* scene, UIManager* ui, PostProcessor* post, DebugOverlay* debug);
    ~Renderer();

    void init();   // загрузка шейдеров и skybox
    void render(float deltaTime);

private:
    SceneManager* m_sceneManager;
    UIManager* m_uiManager;
    PostProcessor* m_postProcessor;
    DebugOverlay* m_debugOverlay;

    Shader* m_lightingShader = nullptr;
    Shader* m_skyboxShader = nullptr;
    Skybox* m_skybox = nullptr;

    // Параметры рендера (можно вынести в настройки)
    bool m_showLightIcons = true;
    float m_lightIconRadius = 0.3f;
};