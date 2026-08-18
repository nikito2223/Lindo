#include "Window.h"
#include "core/Input.h"
#include "Graphics/ui/UIManager.h"
#include "Core/Types/Settings.h"
#include <glad/glad.h>
#include "Core/RenderCommand.h"
#include "debug/DebugLogger.h"
#include <iostream>
#include <debug/Console.h>

namespace Lindo {

    Window::Window(int width, int height, const char* title) {
        DisplaySettings& displaySettings = DisplaySettings::getInstance();
        Settings& settings = Settings::getInstance();

        LOG_INFO("[Window] Initializing Window creation...");
        LOG_INFO(std::string("[Window] Target Resolution from Settings: ") + std::to_string(displaySettings.windowWidth) + "x" + std::to_string(displaySettings.windowHeight));
        LOG_INFO(std::string("[Window] VSync setting is: ") + (displaySettings.vsync ? "ENABLED (1)" : "DISABLED (0)"));
        LOG_INFO(std::string("[Window] MSAA Samples: ") + std::to_string(settings.msaaSamples));
        LOG_INFO(std::string("[Window] Fullscreen mode: ") + (displaySettings.fullscreen ? "ON" : "OFF"));
        LOG_INFO(std::string("[Window] Borderless mode: ") + (displaySettings.borderless ? "ON" : "OFF"));

        m_width = displaySettings.windowWidth;
        m_height = displaySettings.windowHeight;

        if (!glfwInit()) {
            LOG_ERROR("[Window] Failed to initialize GLFW!");
            std::cerr << "Failed to initialize GLFW" << std::endl;
            throw std::runtime_error("GLFW init failed");
        }
        LOG_INFO("[Window] GLFW initialized successfully.");

        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_SAMPLES, settings.msaaSamples);

        if (displaySettings.borderless) {
            LOG_INFO("[Window] Applying window hint: Borderless (GLFW_DECORATED = FALSE)");
            glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
        }

        GLFWmonitor* monitor = displaySettings.fullscreen ? glfwGetPrimaryMonitor() : nullptr;
        m_window = glfwCreateWindow(m_width, m_height, title, monitor, nullptr);

        if (!m_window) {
            LOG_ERROR("[Window] Failed to create GLFW window!");
            glfwTerminate();
            throw std::runtime_error("Failed to create GLFW window");
        }
        LOG_INFO("[Window] GLFW window created successfully.");

        glfwSetWindowUserPointer(m_window, this);
        glfwMakeContextCurrent(m_window);
        LOG_INFO("[Window] OpenGL context made current.");

        // Применяем VSync
        setVSync(displaySettings.vsync);

        glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        LOG_INFO("[Window] Input mode set to CURSOR_DISABLED.");
    }

    Window::~Window() {
        LOG_INFO("[Window] Destroying window and terminating GLFW...");
        if (m_window) {
            glfwDestroyWindow(m_window);
        }
        glfwTerminate();
        LOG_INFO("[Window] Window destroyed.");
    }

    bool Window::shouldClose() const {
        return glfwWindowShouldClose(m_window);
    }

    void Window::swapBuffers() {
        glfwSwapBuffers(m_window);
    }

    void Window::pollEvents() {
        glfwPollEvents();
    }

    void Window::setVSync(bool enabled) {
        DisplaySettings::getInstance().vsync = enabled;
        int swapInterval = enabled ? 1 : 0;
        glfwSwapInterval(swapInterval);
        LOG_INFO(std::string("[Window] glfwSwapInterval updated: ") + std::to_string(swapInterval));
    }

    void Window::setFullscreen(bool fullscreen) {
        LOG_INFO(std::string("[Window] setFullscreen called: ") + (fullscreen ? "true" : "false"));
        GLFWmonitor* monitor = glfwGetPrimaryMonitor();
        const GLFWvidmode* mode = glfwGetVideoMode(monitor);
        DisplaySettings& displaySettings = DisplaySettings::getInstance();

        if (fullscreen && !m_isFullscreen) {
            LOG_INFO("[Window] Switching to Fullscreen mode.");
            glfwGetWindowPos(m_window, &m_windowedX, &m_windowedY);
            glfwGetWindowSize(m_window, &m_windowedW, &m_windowedH);
            glfwSetWindowMonitor(m_window, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
            m_width = mode->width;
            m_height = mode->height;
            m_isFullscreen = true;
            displaySettings.fullscreen = true;
        }
        else if (!fullscreen && m_isFullscreen) {
            LOG_INFO("[Window] Restoring Windowed mode.");
            glfwSetWindowMonitor(m_window, nullptr, m_windowedX, m_windowedY, m_windowedW, m_windowedH, 0);
            m_width = m_windowedW;
            m_height = m_windowedH;
            m_isFullscreen = false;
            displaySettings.fullscreen = false;
        }

        // Повторно восстанавливаем VSync после изменения состояния монитора
        setVSync(displaySettings.vsync);
    }

    void Window::toggleFullscreen() {
        LOG_INFO("[Window] toggleFullscreen triggered.");
        setFullscreen(!m_isFullscreen);
        framebufferSizeCallback(m_window, m_width, m_height);
    }

    void Window::setCallbacks(Lindo::Input::Input* input, Lindo::Graphics::UI::UIManager* uiManager) {
        LOG_INFO("[Window] Registering window callbacks...");
        m_input = input;
        m_uiManager = uiManager;

        glfwSetFramebufferSizeCallback(m_window, framebufferSizeCallback);
        glfwSetCursorPosCallback(m_window, cursorPosCallback);
        glfwSetMouseButtonCallback(m_window, mouseButtonCallback);
        glfwSetScrollCallback(m_window, scrollCallback);
        glfwSetKeyCallback(m_window, keyCallback);

        glfwSetCharCallback(m_window, [](GLFWwindow* window, unsigned int codepoint) {
            Window* win = static_cast<Window*>(glfwGetWindowUserPointer(window));
            if (win && win->m_input) {
                win->m_input->onChar(codepoint);
            }
            });

        LOG_INFO("[Window] Callbacks registered successfully.");
    }

    void Window::framebufferSizeCallback(GLFWwindow* window, int w, int h) {
        if (h == 0) h = 1;

        LOG_DEBUG(std::string("[Window] Framebuffer resized to: ") + std::to_string(w) + "x" + std::to_string(h));

        Lindo::Graphics::RenderCommand::SetViewport(0, 0, w, h);

        DisplaySettings& displaySettings = DisplaySettings::getInstance();
        displaySettings.windowWidth = w;
        displaySettings.windowHeight = h;

        Window* win = static_cast<Window*>(glfwGetWindowUserPointer(window));
        if (!win) return;

        win->m_width = w;
        win->m_height = h;

        if (win->m_uiManager) win->m_uiManager->onResize(w, h);
        if (win->m_resizeCallback) win->m_resizeCallback(w, h);
    }

    void Window::cursorPosCallback(GLFWwindow* window, double x, double y) {
        Window* win = static_cast<Window*>(glfwGetWindowUserPointer(window));
        if (!win) return;

        if (win->m_input) win->m_input->onMouseMove(x, y);
        if (win->m_uiManager) win->m_uiManager->onMouseMove((float)x, (float)y);
    }

    void Window::mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
        Window* win = static_cast<Window*>(glfwGetWindowUserPointer(window));
        if (!win) return;

        if (win->m_input) win->m_input->onMouseButton(button, action);
        if (win->m_uiManager) {
            double x, y;
            glfwGetCursorPos(window, &x, &y);
            win->m_uiManager->onMouseButton((float)x, (float)y, button, action == GLFW_PRESS);
        }
    }

    void Window::scrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
        Window* win = static_cast<Window*>(glfwGetWindowUserPointer(window));
        if (win && win->m_input) {
            win->m_input->onScroll(yoffset);
        }
    }

    void Window::keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
        Window* win = static_cast<Window*>(glfwGetWindowUserPointer(window));
        if (!win) return;

        if (win->m_input) {
            win->m_input->onKey(key, action);
        }

        if (action == GLFW_PRESS || action == GLFW_REPEAT) {
            bool ctrlPressed = (mods & GLFW_MOD_CONTROL) != 0;

            if (win->m_uiManager) {
                auto* console = win->m_uiManager->getConsole();
                if (console && console->isVisible()) {
                    if (key == GLFW_KEY_TAB) {
                        console->onTab();
                    }
                    else if (ctrlPressed) {
                        if (key == GLFW_KEY_A) console->onSelectAll();
                        else if (key == GLFW_KEY_C) console->onCopy();
                        else if (key == GLFW_KEY_V) console->onPaste();
                        else if (key == GLFW_KEY_X) console->onCut();
                    }
                }
            }
        }
    }

    void Window::showSplashScreen(int width, int height, const std::string& title) {
        LOG_INFO("[Window] Showing splash screen...");
        glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
        GLFWwindow* splash = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
        if (!splash) {
            LOG_ERROR("[Window] Failed to create splash screen window!");
            return;
        }

        glfwMakeContextCurrent(splash);

        Lindo::Graphics::RenderCommand::SetClearColor(glm::vec4(0.15f, 0.15f, 0.15f, 1.0f));
        Lindo::Graphics::RenderCommand::Clear(true, true);
        glfwSwapBuffers(splash);
        glfwPollEvents();

        glfwMakeContextCurrent(nullptr);
        glfwDestroyWindow(splash);
        glfwDefaultWindowHints();
        LOG_INFO("[Window] Splash screen closed.");
    }

    void Window::setResizeCallback(std::function<void(int, int)> callback) {
        m_resizeCallback = callback;
    }
}