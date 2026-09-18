#pragma once

#ifdef _WIN32
#  ifndef WIN32_LEAN_AND_MEAN
#    define WIN32_LEAN_AND_MEAN
#  endif
#  ifndef NOMINMAX
#    define NOMINMAX
#  endif
#  include <windows.h>
#endif

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <string>
#include <functional>

namespace Lindo {
    namespace Input { class Input; }
    namespace Graphics { namespace UI { class UIManager; } }
}

namespace Lindo {

    class Window {
    public:
        Window(int width, int height, const char* title);
        ~Window();

        bool shouldClose() const;
        void swapBuffers();
        void pollEvents();

        void setVSync(bool enabled);

        int getWidth() const { return m_width; }
        int getHeight() const { return m_height; }

        GLFWwindow* getNativeWindow() const { return m_window; }
        GLFWwindow* getHandle() const { return m_window; }
        GLFWwindow* getGLFWwindow() const { return m_window; }

        void setFullscreen(bool fullscreen);
        void toggleFullscreen();

        void setCallbacks(Lindo::Input::Input* input, Lindo::Graphics::UI::UIManager* uiManager);
        void setResizeCallback(std::function<void(int, int)> callback);

        static void showSplashScreen(int width, int height, const std::string& title);
        static void windowSizeCallback(GLFWwindow* window, int width, int height);

    private:
        GLFWwindow* m_window = nullptr;
        int m_width = 0;
        int m_height = 0;

        int m_windowedX = 0;
        int m_windowedY = 0;
        int m_windowedW = 0;
        int m_windowedH = 0;
        bool m_isFullscreen = false;

        std::function<void(int, int)> m_resizeCallback;

        Lindo::Input::Input* m_input = nullptr;
        Lindo::Graphics::UI::UIManager* m_uiManager = nullptr;

        static void framebufferSizeCallback(GLFWwindow* window, int w, int h);
        static void cursorPosCallback(GLFWwindow* window, double x, double y);
        static void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
        static void scrollCallback(GLFWwindow* window, double xoffset, double yoffset);
        static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
    };

}