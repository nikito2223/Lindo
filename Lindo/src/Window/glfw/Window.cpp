#include "Window.h"
#include "core/Input.h"
#include "Graphics/ui/UIManager.h"
#include <iostream>
#include <core/Globals.h>

Input* Window::s_inputInstance = nullptr;
UIManager* Window::s_uiInstance = nullptr;

Window::Window(int width, int height, const char* title)
    : m_width(width), m_height(height) {
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        throw std::runtime_error("GLFW init failed");
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_SAMPLES, 4);

    m_window = glfwCreateWindow(width, height, title, nullptr, nullptr);
    if (!m_window) {
        glfwTerminate();
        throw std::runtime_error("Failed to create GLFW window");
    }

    glfwSetWindowUserPointer(m_window, this);
    glfwMakeContextCurrent(m_window);
    glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
}

Window::~Window() {
    glfwDestroyWindow(m_window);
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

void Window::setCallbacks(Input* input, UIManager* uiManager) {
    s_inputInstance = input;
    s_uiInstance = uiManager;

    glfwSetFramebufferSizeCallback(m_window, framebufferSizeCallback);
    glfwSetCursorPosCallback(m_window, cursorPosCallback);
    glfwSetMouseButtonCallback(m_window, mouseButtonCallback);
    glfwSetScrollCallback(m_window, scrollCallback);
    glfwSetKeyCallback(m_window, keyCallback);
}

void Window::framebufferSizeCallback(GLFWwindow* window, int w, int h) {
    if (h == 0) h = 1; // защита от Alt+Tab / свернутого окна
    glViewport(0, 0, w, h);

    SCR_WIDTH = w;
    SCR_HEIGHT = h;

    if (s_uiInstance) s_uiInstance->onResize(w, h);

    Window* win = static_cast<Window*>(glfwGetWindowUserPointer(window));
    if (win && win->m_resizeCallback) win->m_resizeCallback(w, h);
}

void Window::cursorPosCallback(GLFWwindow* window, double x, double y) {
    if (s_inputInstance) {
        s_inputInstance->onMouseMove(x, y);
    }
    if (s_uiInstance) {
        s_uiInstance->onMouseMove((float)x, (float)y);
    }
}

void Window::mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
    if (s_inputInstance) {
        s_inputInstance->onMouseButton(button, action);
    }
    if (s_uiInstance) {
        double x, y;
        glfwGetCursorPos(window, &x, &y);
        s_uiInstance->onMouseButton((float)x, (float)y, button, action == GLFW_PRESS);
    }
}

void Window::scrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
    if (s_inputInstance) {
        s_inputInstance->onScroll(yoffset);
    }
}

void Window::keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (s_inputInstance) {
        s_inputInstance->onKey(key, action);
    }
}
void Window::setResizeCallback(std::function<void(int, int)> callback) {
    m_resizeCallback = callback;
}