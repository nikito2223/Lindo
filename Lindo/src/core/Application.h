#pragma once
#include <memory>

class Window;
class Input;
class SceneManager;
class UIManager;
class PostProcessor;
class DebugOverlay;
class Renderer;

class Application {
public:
    Application();
    ~Application();
    void onResize(int width, int height);


    void run();

private:
    std::unique_ptr<Window> m_window;
    std::unique_ptr<Input> m_input;
    std::unique_ptr<SceneManager> m_sceneManager;
    std::unique_ptr<UIManager> m_uiManager;
    std::unique_ptr<PostProcessor> m_postProcessor;
    std::unique_ptr<DebugOverlay> m_debugOverlay;
    std::unique_ptr<Renderer> m_renderer;

    float m_lastFrameTime = 0.0f;
};