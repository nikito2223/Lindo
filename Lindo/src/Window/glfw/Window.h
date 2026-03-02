#pragma once
#include <core/OGL.h>
#include <string>

class Input;
class UIManager;

class Window {
public:
    Window(int width, int height, const char* title);
    ~Window();

    bool shouldClose() const;
    void swapBuffers();
    void pollEvents();
    int getWidth() const { return m_width; }
    int getHeight() const { return m_height; }
    GLFWwindow* getHandle() const { return m_window; }

    void setFullscreen(bool fullscreen);
    void toggleFullscreen();

    // Установка обработчиков (вызывается после создания окна)
    void setCallbacks(Input* input, UIManager* uiManager);

private:
    GLFWwindow* m_window;
    int m_width, m_height;
    int m_windowedX, m_windowedY, m_windowedW, m_windowedH;
    bool m_isFullscreen = false;

    static Input* s_inputInstance;
    static UIManager* s_uiInstance;

    static void framebufferSizeCallback(GLFWwindow* window, int w, int h);
    static void cursorPosCallback(GLFWwindow* window, double x, double y);
    static void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
    static void scrollCallback(GLFWwindow* window, double xoffset, double yoffset);
    static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
};