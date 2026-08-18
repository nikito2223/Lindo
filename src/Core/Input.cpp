#include "Input.h"
#include "Window/Window.h"
#include <debug/Console.h>
#include <GLFW/glfw3.h>

namespace Lindo {
    namespace Input {

        bool Input::isConsoleActive() const {
            return m_console && m_console->isVisible();
        }

        void Input::update(Lindo::Window* window) {
            bool consoleOpen = isConsoleActive();
            int cursorMode = (m_uiActive || consoleOpen) ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED;
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

        bool Input::consumeF4() {
            if (m_f4Pressed && !m_f4Consumed) {
                m_f4Consumed = true;
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
            if (active) {
                m_firstMouse = true;
            }
        }

        void Input::clearMovementKeys() {
            m_forward = m_backward = m_left = m_right = false;
            m_jump = false;
            m_crouch = false;
        }

        void Input::handleConsoleKey(int key) {
            if (!m_console) return;

            switch (key) {
            case GLFW_KEY_BACKSPACE: m_console->onBackspace(); break;
            case GLFW_KEY_DELETE:    m_console->onDeleteForward(); break;
            case GLFW_KEY_ENTER:
            case GLFW_KEY_KP_ENTER:  m_console->onEnter(); break;
            case GLFW_KEY_TAB:       m_console->onTab(); break;
            case GLFW_KEY_ESCAPE:    m_console->onEscape(); break;
            case GLFW_KEY_HOME:      m_console->onHome(); break;
            case GLFW_KEY_END:       m_console->onEnd(); break;
            case GLFW_KEY_LEFT:      m_console->onMoveCursorLeft(); break;
            case GLFW_KEY_RIGHT:     m_console->onMoveCursorRight(); break;
            case GLFW_KEY_UP:        m_console->onHistoryUp(); break;
            case GLFW_KEY_DOWN:      m_console->onHistoryDown(); break;
            }
        }

        void Input::onKey(int key, int action) {
            bool pressed = (action == GLFW_PRESS);
            bool repeated = (action == GLFW_REPEAT);

            // Клавиша ~ (`) открывает/закрывает консоль независимо от текущего режима игры/UI
            if (key == GLFW_KEY_GRAVE_ACCENT) {
                if (pressed && m_console) {
                    m_console->toggle();
                    if (m_console->isVisible()) {
                        clearMovementKeys(); // чтобы зажатая WASD не "залипала" после открытия консоли
                    }
                }
                return;
            }

            if (isConsoleActive()) {
                // Пока консоль открыта - вся клавиатура принадлежит ей, в движок ничего не идёт
                if (pressed || repeated) {
                    handleConsoleKey(key);
                }
                return;
            }

            if (repeated) return; // автоповтор для игровых клавиш (тоггл-стиль) не нужен

            switch (key) {
            case GLFW_KEY_W: m_forward = pressed; break;
            case GLFW_KEY_S: m_backward = pressed; break;
            case GLFW_KEY_A: m_left = pressed; break;
            case GLFW_KEY_D: m_right = pressed; break;
            case GLFW_KEY_SPACE: m_jump = pressed; break;
            case GLFW_KEY_LEFT_SHIFT: m_crouch = pressed; break;
            case GLFW_KEY_F3:
                m_f3Pressed = pressed;
                if (pressed) m_f3Consumed = false;
                break;
            case GLFW_KEY_F4:
                m_f4Pressed = pressed;
                if (pressed) m_f4Consumed = false;
                break;
            case GLFW_KEY_F11:
                m_f11Pressed = pressed;
                if (pressed) m_f11Consumed = false;
                break;
            case GLFW_KEY_ESCAPE:
                m_escapePressed = pressed;
                if (pressed) m_escapeConsumed = false;
                break;
            }
        }

        void Input::onChar(unsigned int codepoint) {
            if (isConsoleActive()) {
                m_console->onChar(codepoint);
            }
        }

        void Input::onMouseMove(double x, double y) {
            if (m_firstMouse) {
                m_lastX = x;
                m_lastY = y;
                m_firstMouse = false;
            }

            float xoffset = static_cast<float>(x - m_lastX);
            float yoffset = static_cast<float>(m_lastY - y);

            m_lastX = x;
            m_lastY = y;

            if (!m_uiActive && !isConsoleActive()) {
                m_mouseDelta += glm::vec2(xoffset, yoffset);
            }
        }

        void Input::onMouseButton(int button, int action) {
        }

        void Input::onScroll(double yoffset) {
            if (isConsoleActive()) {
                m_console->onScroll(static_cast<float>(yoffset));
                return;
            }
            m_scrollY += static_cast<float>(yoffset);
        }
    }
}