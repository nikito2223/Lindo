#include "Input.h"
#include "Window/glfw/Window.h"
#include <GLFW/glfw3.h>

void Input::update(Window* window) {
    int cursorMode = m_uiActive ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED;
    glfwSetInputMode(window->getHandle(), GLFW_CURSOR, cursorMode);
}

bool Input::consumeF3() {
    if (m_f3Pressed && !m_f3Consumed) {
        m_f3Consumed = true;
        return true;
    }
    return false;
}

bool Input::consumeF11() {
    if (m_f11Pressed && !m_f11Consumed) {
        m_f11Consumed = true;
        return true;
    }
    return false;
}

bool Input::consumeEscape() {
    if (m_escapePressed && !m_escapeConsumed) {
        m_escapeConsumed = true;
        return true;
    }
    return false;
}

void Input::setUIActive(bool active) {
    m_uiActive = active;
    // При активации UI сбрасываем флаги мыши, чтобы не было резкого перемещения
    if (active) {
        m_firstMouse = true;
    }
}

void Input::onKey(int key, int action) {
    bool pressed = (action == GLFW_PRESS || action == GLFW_REPEAT);

    switch (key) {
    case GLFW_KEY_W: m_forward = pressed; break;
    case GLFW_KEY_S: m_backward = pressed; break;
    case GLFW_KEY_A: m_left = pressed; break;
    case GLFW_KEY_D: m_right = pressed; break;
    case GLFW_KEY_SPACE: m_jump = pressed; break;
    case GLFW_KEY_LEFT_SHIFT: m_crouch = pressed; break;
    case GLFW_KEY_F3:
        if (pressed) {
            m_f3Pressed = true;
            m_f3Consumed = false;
        }
        else {
            m_f3Pressed = false;
        }
        break;
    case GLFW_KEY_F11:
        if (pressed) {
            m_f11Pressed = true;
            m_f11Consumed = false;
        }
        else {
            m_f11Pressed = false;
        }
        break;
    case GLFW_KEY_ESCAPE:
        if (pressed) {
            m_escapePressed = true;
            m_escapeConsumed = false;
        }
        else {
            m_escapePressed = false;
        }
        break;
    }
}

void Input::onMouseMove(double x, double y) {
    if (m_firstMouse) {
        m_lastX = x;
        m_lastY = y;
        m_firstMouse = false;
    }

    float xoffset = static_cast<float>(x - m_lastX);
    float yoffset = static_cast<float>(m_lastY - y); // инвертировано, т.к. y идёт снизу вверх

    m_lastX = x;
    m_lastY = y;

    if (!m_uiActive) {
        m_mouseDelta += glm::vec2(xoffset, yoffset);
    }
}

void Input::onMouseButton(int button, int action) {
    // Пока не используем, но можно добавить
}

void Input::onScroll(double yoffset) {
    m_scrollY += static_cast<float>(yoffset);
}