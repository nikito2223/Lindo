#include "Window.h"
#include "core/Input.h"
#include "Graphics/ui/UIManager.h"
#include "core/Globals.h"
#include <glad/glad.h>
#include <iostream>

namespace Lindo {

    Window::Window(int width, int height, const char* title)
        : m_width(width), m_height(height) {

        if (!glfwInit()) {
            std::cerr << "Failed to initialize GLFW" << std::endl;
            throw std::runtime_error("GLFW init failed");
        }

        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_SAMPLES, 4); // Исправлено со значения 3 на валидную степень 2

        m_window = glfwCreateWindow(width, height, title, nullptr, nullptr);
        if (!m_window) {
            glfwTerminate();
            throw std::runtime_error("Failed to create GLFW window");
        }

        glfwSetWindowUserPointer(m_window, this);
        glfwMakeContextCurrent(m_window);
        glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

        // Инициализация GLAD прямо при создании контекста окна
        if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
            std::cerr << "Failed to initialize GLAD" << std::endl;
            throw std::runtime_error("GLAD init failed");
        }

        // Вывод информации о GPU в консоль для отладки
        std::cout << "[GPU Vendor]   : " << glGetString(GL_VENDOR) << std::endl;
        std::cout << "[GPU Renderer] : " << glGetString(GL_RENDERER) << std::endl;
    }

    Window::~Window() {
        if (m_window) {
            glfwDestroyWindow(m_window);
        }
        glfwTerminate();
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

    void Window::setFullscreen(bool fullscreen) {
        GLFWmonitor* monitor = glfwGetPrimaryMonitor();
        const GLFWvidmode* mode = glfwGetVideoMode(monitor);

        if (fullscreen && !m_isFullscreen) {
            glfwGetWindowPos(m_window, &m_windowedX, &m_windowedY);
            glfwGetWindowSize(m_window, &m_windowedW, &m_windowedH);
            glfwSetWindowMonitor(m_window, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
            m_width = mode->width;
            m_height = mode->height;
            m_isFullscreen = true;
        }
        else if (!fullscreen && m_isFullscreen) {
            glfwSetWindowMonitor(m_window, nullptr, m_windowedX, m_windowedY, m_windowedW, m_windowedH, 0);
            m_width = m_windowedW;
            m_height = m_windowedH;
            m_isFullscreen = false;
        }
    }

    void Window::toggleFullscreen() {
        setFullscreen(!m_isFullscreen);
        framebufferSizeCallback(m_window, m_width, m_height);
    }

    void Window::setCallbacks(Lindo::Input::Input* input, Lindo::Graphics::UI::UIManager* uiManager) {
        m_input = input;      // Сохраняем указатели в сам экземпляр класса
        m_uiManager = uiManager;

        glfwSetFramebufferSizeCallback(m_window, framebufferSizeCallback);
        glfwSetCursorPosCallback(m_window, cursorPosCallback);
        glfwSetMouseButtonCallback(m_window, mouseButtonCallback);
        glfwSetScrollCallback(m_window, scrollCallback);
        glfwSetKeyCallback(m_window, keyCallback);
    }

    void Window::framebufferSizeCallback(GLFWwindow* window, int w, int h) {
        if (h == 0) h = 1;
        glViewport(0, 0, w, h);

        Globals::screenWidth = w;
        Globals::screenHeight = h;

        Window* win = static_cast<Window*>(glfwGetWindowUserPointer(window));
        if (!win) return;

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
        if (win && win->m_input) {
            win->m_input->onKey(key, action);
        }
    }

    void Window::showSplashScreen(int width, int height, const std::string& title) {
        glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
        GLFWwindow* splash = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
        if (!splash) return;

        glfwMakeContextCurrent(splash);

        if (gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
            glClearColor(0.15f, 0.15f, 0.15f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);
            glfwSwapBuffers(splash);
            glfwPollEvents();
        }

        glfwMakeContextCurrent(nullptr); // Отвязываем контекст перед уничтожением
        glfwDestroyWindow(splash);
        glfwDefaultWindowHints();
    }

    void Window::setResizeCallback(std::function<void(int, int)> callback) {
        m_resizeCallback = callback;
    }
}