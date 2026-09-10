#include "Input.h"
#include "Window/Window.h"
#include <debug/Console.h>
#include <GLFW/glfw3.h>
#include <algorithm>
#include <cctype>

namespace Lindo {
    namespace Input {

        Input* Input::s_Instance = nullptr;

        static std::string toLower(std::string str) {
            std::transform(str.begin(), str.end(), str.begin(), [](unsigned char c) {
                return static_cast<char>(std::tolower(c));
                });
            return str;
        }

        Input::Input() {
            s_Instance = this;
            initKeyMappings();
        }

        void Input::initKeyMappings() {
            // --- Буквы ---
            m_glfwToKeyMap[GLFW_KEY_A] = KeyCode::A; m_glfwToKeyMap[GLFW_KEY_B] = KeyCode::B;
            m_glfwToKeyMap[GLFW_KEY_C] = KeyCode::C; m_glfwToKeyMap[GLFW_KEY_D] = KeyCode::D;
            m_glfwToKeyMap[GLFW_KEY_E] = KeyCode::E; m_glfwToKeyMap[GLFW_KEY_F] = KeyCode::F;
            m_glfwToKeyMap[GLFW_KEY_G] = KeyCode::G; m_glfwToKeyMap[GLFW_KEY_H] = KeyCode::H;
            m_glfwToKeyMap[GLFW_KEY_I] = KeyCode::I; m_glfwToKeyMap[GLFW_KEY_J] = KeyCode::J;
            m_glfwToKeyMap[GLFW_KEY_K] = KeyCode::K; m_glfwToKeyMap[GLFW_KEY_L] = KeyCode::L;
            m_glfwToKeyMap[GLFW_KEY_M] = KeyCode::M; m_glfwToKeyMap[GLFW_KEY_N] = KeyCode::N;
            m_glfwToKeyMap[GLFW_KEY_O] = KeyCode::O; m_glfwToKeyMap[GLFW_KEY_P] = KeyCode::P;
            m_glfwToKeyMap[GLFW_KEY_Q] = KeyCode::Q; m_glfwToKeyMap[GLFW_KEY_R] = KeyCode::R;
            m_glfwToKeyMap[GLFW_KEY_S] = KeyCode::S; m_glfwToKeyMap[GLFW_KEY_T] = KeyCode::T;
            m_glfwToKeyMap[GLFW_KEY_U] = KeyCode::U; m_glfwToKeyMap[GLFW_KEY_V] = KeyCode::V;
            m_glfwToKeyMap[GLFW_KEY_W] = KeyCode::W; m_glfwToKeyMap[GLFW_KEY_X] = KeyCode::X;
            m_glfwToKeyMap[GLFW_KEY_Y] = KeyCode::Y; m_glfwToKeyMap[GLFW_KEY_Z] = KeyCode::Z;

            // --- Цифры основного ряда ---
            m_glfwToKeyMap[GLFW_KEY_0] = KeyCode::Alpha0; m_glfwToKeyMap[GLFW_KEY_1] = KeyCode::Alpha1;
            m_glfwToKeyMap[GLFW_KEY_2] = KeyCode::Alpha2; m_glfwToKeyMap[GLFW_KEY_3] = KeyCode::Alpha3;
            m_glfwToKeyMap[GLFW_KEY_4] = KeyCode::Alpha4; m_glfwToKeyMap[GLFW_KEY_5] = KeyCode::Alpha5;
            m_glfwToKeyMap[GLFW_KEY_6] = KeyCode::Alpha6; m_glfwToKeyMap[GLFW_KEY_7] = KeyCode::Alpha7;
            m_glfwToKeyMap[GLFW_KEY_8] = KeyCode::Alpha8; m_glfwToKeyMap[GLFW_KEY_9] = KeyCode::Alpha9;

            // --- Numpad (Keypad) ---
            m_glfwToKeyMap[GLFW_KEY_KP_0] = KeyCode::Keypad0; m_glfwToKeyMap[GLFW_KEY_KP_1] = KeyCode::Keypad1;
            m_glfwToKeyMap[GLFW_KEY_KP_2] = KeyCode::Keypad2; m_glfwToKeyMap[GLFW_KEY_KP_3] = KeyCode::Keypad3;
            m_glfwToKeyMap[GLFW_KEY_KP_4] = KeyCode::Keypad4; m_glfwToKeyMap[GLFW_KEY_KP_5] = KeyCode::Keypad5;
            m_glfwToKeyMap[GLFW_KEY_KP_6] = KeyCode::Keypad6; m_glfwToKeyMap[GLFW_KEY_KP_7] = KeyCode::Keypad7;
            m_glfwToKeyMap[GLFW_KEY_KP_8] = KeyCode::Keypad8; m_glfwToKeyMap[GLFW_KEY_KP_9] = KeyCode::Keypad9;
            m_glfwToKeyMap[GLFW_KEY_KP_DIVIDE] = KeyCode::KeypadDivide;
            m_glfwToKeyMap[GLFW_KEY_KP_MULTIPLY] = KeyCode::KeypadMultiply;
            m_glfwToKeyMap[GLFW_KEY_KP_SUBTRACT] = KeyCode::KeypadSubtract;
            m_glfwToKeyMap[GLFW_KEY_KP_ADD] = KeyCode::KeypadAdd;
            m_glfwToKeyMap[GLFW_KEY_KP_ENTER] = KeyCode::KeypadEnter;
            m_glfwToKeyMap[GLFW_KEY_KP_DECIMAL] = KeyCode::KeypadDecimal;

            // --- Управление и модификаторы ---
            m_glfwToKeyMap[GLFW_KEY_SPACE] = KeyCode::Space;
            m_glfwToKeyMap[GLFW_KEY_ENTER] = KeyCode::Enter;
            m_glfwToKeyMap[GLFW_KEY_ESCAPE] = KeyCode::Escape;
            m_glfwToKeyMap[GLFW_KEY_TAB] = KeyCode::Tab;
            m_glfwToKeyMap[GLFW_KEY_BACKSPACE] = KeyCode::Backspace;
            m_glfwToKeyMap[GLFW_KEY_DELETE] = KeyCode::Delete;
            m_glfwToKeyMap[GLFW_KEY_INSERT] = KeyCode::Insert;
            m_glfwToKeyMap[GLFW_KEY_GRAVE_ACCENT] = KeyCode::Tilde;

            m_glfwToKeyMap[GLFW_KEY_LEFT_SHIFT] = KeyCode::LeftShift;
            m_glfwToKeyMap[GLFW_KEY_RIGHT_SHIFT] = KeyCode::RightShift;
            m_glfwToKeyMap[GLFW_KEY_LEFT_CONTROL] = KeyCode::LeftControl;
            m_glfwToKeyMap[GLFW_KEY_RIGHT_CONTROL] = KeyCode::RightControl;
            m_glfwToKeyMap[GLFW_KEY_LEFT_ALT] = KeyCode::LeftAlt;
            m_glfwToKeyMap[GLFW_KEY_RIGHT_ALT] = KeyCode::RightAlt;

            // --- F-клавиши ---
            m_glfwToKeyMap[GLFW_KEY_F1] = KeyCode::F1;  m_glfwToKeyMap[GLFW_KEY_F2] = KeyCode::F2;
            m_glfwToKeyMap[GLFW_KEY_F3] = KeyCode::F3;  m_glfwToKeyMap[GLFW_KEY_F4] = KeyCode::F4;
            m_glfwToKeyMap[GLFW_KEY_F5] = KeyCode::F5;  m_glfwToKeyMap[GLFW_KEY_F6] = KeyCode::F6;
            m_glfwToKeyMap[GLFW_KEY_F7] = KeyCode::F7;  m_glfwToKeyMap[GLFW_KEY_F8] = KeyCode::F8;
            m_glfwToKeyMap[GLFW_KEY_F9] = KeyCode::F9;  m_glfwToKeyMap[GLFW_KEY_F10] = KeyCode::F10;
            m_glfwToKeyMap[GLFW_KEY_F11] = KeyCode::F11; m_glfwToKeyMap[GLFW_KEY_F12] = KeyCode::F12;

            // --- Навигация ---
            m_glfwToKeyMap[GLFW_KEY_UP] = KeyCode::UpArrow;
            m_glfwToKeyMap[GLFW_KEY_DOWN] = KeyCode::DownArrow;
            m_glfwToKeyMap[GLFW_KEY_LEFT] = KeyCode::LeftArrow;
            m_glfwToKeyMap[GLFW_KEY_RIGHT] = KeyCode::RightArrow;
            m_glfwToKeyMap[GLFW_KEY_HOME] = KeyCode::Home;
            m_glfwToKeyMap[GLFW_KEY_END] = KeyCode::End;
            m_glfwToKeyMap[GLFW_KEY_PAGE_UP] = KeyCode::PageUp;
            m_glfwToKeyMap[GLFW_KEY_PAGE_DOWN] = KeyCode::PageDown;

            // --- Символы и пунктуация ---
            m_glfwToKeyMap[GLFW_KEY_MINUS] = KeyCode::Minus;
            m_glfwToKeyMap[GLFW_KEY_EQUAL] = KeyCode::Equal;
            m_glfwToKeyMap[GLFW_KEY_LEFT_BRACKET] = KeyCode::LeftBracket;
            m_glfwToKeyMap[GLFW_KEY_RIGHT_BRACKET] = KeyCode::RightBracket;
            m_glfwToKeyMap[GLFW_KEY_SEMICOLON] = KeyCode::Semicolon;
            m_glfwToKeyMap[GLFW_KEY_APOSTROPHE] = KeyCode::Apostrophe;
            m_glfwToKeyMap[GLFW_KEY_COMMA] = KeyCode::Comma;
            m_glfwToKeyMap[GLFW_KEY_PERIOD] = KeyCode::Period;
            m_glfwToKeyMap[GLFW_KEY_SLASH] = KeyCode::Slash;
            m_glfwToKeyMap[GLFW_KEY_BACKSLASH] = KeyCode::Backslash;

            // --- Системные клавиши ---
            m_glfwToKeyMap[GLFW_KEY_CAPS_LOCK] = KeyCode::CapsLock;
            m_glfwToKeyMap[GLFW_KEY_NUM_LOCK] = KeyCode::NumLock;
            m_glfwToKeyMap[GLFW_KEY_SCROLL_LOCK] = KeyCode::ScrollLock;
            m_glfwToKeyMap[GLFW_KEY_PRINT_SCREEN] = KeyCode::PrintScreen;
            m_glfwToKeyMap[GLFW_KEY_PAUSE] = KeyCode::Pause;

            // --- Буквенные алиасы ---
            bindAction("a", KeyCode::A); bindAction("b", KeyCode::B); bindAction("c", KeyCode::C);
            bindAction("d", KeyCode::D); bindAction("e", KeyCode::E); bindAction("f", KeyCode::F);
            bindAction("g", KeyCode::G); bindAction("h", KeyCode::H); bindAction("i", KeyCode::I);
            bindAction("j", KeyCode::J); bindAction("k", KeyCode::K); bindAction("l", KeyCode::L);
            bindAction("m", KeyCode::M); bindAction("n", KeyCode::N); bindAction("o", KeyCode::O);
            bindAction("p", KeyCode::P); bindAction("q", KeyCode::Q); bindAction("r", KeyCode::R);
            bindAction("s", KeyCode::S); bindAction("t", KeyCode::T); bindAction("u", KeyCode::U);
            bindAction("v", KeyCode::V); bindAction("w", KeyCode::W); bindAction("x", KeyCode::X);
            bindAction("y", KeyCode::Y); bindAction("z", KeyCode::Z);

            // --- Экшены ---
            bindAction("space", KeyCode::Space);
            bindAction("jump", KeyCode::Space);
            bindAction("walk", KeyCode::W);
            bindAction("run", KeyCode::LeftShift);
            bindAction("crouch", KeyCode::LeftControl);
            bindAction("move_forward", KeyCode::W);
            bindAction("move_backward", KeyCode::S);
            bindAction("move_left", KeyCode::A);
            bindAction("move_right", KeyCode::D);
            bindAction("sprint", KeyCode::LeftShift);
            bindAction("free_camera", KeyCode::F5);
            bindAction("free_up", KeyCode::Space);
            bindAction("free_down", KeyCode::LeftControl);
            bindAction("fire0", KeyCode::Mouse0);
            bindAction("fire1", KeyCode::Mouse1);
            bindAction("escape", KeyCode::Escape);
            bindAction("pause", KeyCode::Escape);
        }

        KeyCode Input::glfwKeyToKeyCode(int glfwKey) const {
            auto it = m_glfwToKeyMap.find(glfwKey);
            if (it != m_glfwToKeyMap.end()) return it->second;
            return KeyCode::None;
        }

        KeyCode Input::glfwButtonToKeyCode(int glfwButton) const {
            switch (glfwButton) {
            case GLFW_MOUSE_BUTTON_LEFT:   return KeyCode::Mouse0;
            case GLFW_MOUSE_BUTTON_RIGHT:  return KeyCode::Mouse1;
            case GLFW_MOUSE_BUTTON_MIDDLE: return KeyCode::Mouse2;
            case GLFW_MOUSE_BUTTON_4:      return KeyCode::Mouse3;
            case GLFW_MOUSE_BUTTON_5:      return KeyCode::Mouse4;
            default: return KeyCode::None;
            }
        }

        void Input::bindAction(const std::string& actionName, KeyCode key) {
            m_stringToKeyMap[toLower(actionName)] = key;
        }

        void Input::update(Lindo::Window* window) {
            std::fill(std::begin(m_keysPressed), std::end(m_keysPressed), false);
            std::fill(std::begin(m_keysReleased), std::end(m_keysReleased), false);

            m_scrollY = 0.0f;
            m_mouseDelta = glm::vec2(0.0f);

            bool consoleOpen = isConsoleActive();
            int cursorMode = (m_uiActive || consoleOpen) ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED;
            glfwSetInputMode(window->getHandle(), GLFW_CURSOR, cursorMode);
        }

        // --- ГЕТТЕРЫ ВВОДА ---

        bool Input::getKey(KeyCode key) const {
            if (key == KeyCode::None) return false;
            return m_keysHeld[static_cast<size_t>(key)];
        }

        bool Input::getKeyDown(KeyCode key) const {
            if (key == KeyCode::None) return false;
            return m_keysPressed[static_cast<size_t>(key)];
        }

        bool Input::getKeyUp(KeyCode key) const {
            if (key == KeyCode::None) return false;
            return m_keysReleased[static_cast<size_t>(key)];
        }

        bool Input::getKey(int glfwKey) const {
            return getKey(glfwKeyToKeyCode(glfwKey));
        }

        bool Input::getKeyDown(int glfwKey) const {
            return getKeyDown(glfwKeyToKeyCode(glfwKey));
        }

        bool Input::getKeyUp(int glfwKey) const {
            return getKeyUp(glfwKeyToKeyCode(glfwKey));
        }

        bool Input::getMouseButton(int button) const {
            return getKey(glfwButtonToKeyCode(button));
        }

        bool Input::getMouseButtonDown(int button) const {
            return getKeyDown(glfwButtonToKeyCode(button));
        }

        bool Input::getMouseButtonUp(int button) const {
            return getKeyUp(glfwButtonToKeyCode(button));
        }

        bool Input::getKey(const std::string& name) const {
            auto it = m_stringToKeyMap.find(toLower(name));
            return (it != m_stringToKeyMap.end()) ? getKey(it->second) : false;
        }

        bool Input::getKeyDown(const std::string& name) const {
            auto it = m_stringToKeyMap.find(toLower(name));
            return (it != m_stringToKeyMap.end()) ? getKeyDown(it->second) : false;
        }

        bool Input::getKeyUp(const std::string& name) const {
            auto it = m_stringToKeyMap.find(toLower(name));
            return (it != m_stringToKeyMap.end()) ? getKeyUp(it->second) : false;
        }

        float Input::getAxis(const std::string& positive, const std::string& negative) const {
            const float positiveValue = getAction(positive) ? 1.0f : 0.0f;
            const float negativeValue = getAction(negative) ? 1.0f : 0.0f;
            return positiveValue - negativeValue;
        }

        bool Input::consumeKey(KeyCode key) {
            if (key == KeyCode::None) return false;
            const size_t index = static_cast<size_t>(key);
            if (!m_keysPressed[index]) return false;
            m_keysPressed[index] = false;
            return true;
        }

        bool Input::consumeKey(int glfwKey) {
            return consumeKey(glfwKeyToKeyCode(glfwKey));
        }

        // --- ОБРАБОТКА GLFW КОЛБЭКОВ ---

        void Input::onKey(int key, int action) {
            KeyCode code = glfwKeyToKeyCode(key);
            if (code == KeyCode::None) return;
        
            if (code == KeyCode::Tilde && action == GLFW_PRESS && m_console) {
                m_console->toggle();
                return;
            }
        
            size_t index = static_cast<size_t>(code);
        
            if (action == GLFW_PRESS) {
                m_keysHeld[index] = true;
                m_keysPressed[index] = true;
            }
            else if (action == GLFW_RELEASE) {
                m_keysHeld[index] = false;
                m_keysReleased[index] = true;
            }
        
            // Пересылаем нажатия клавиш зарегистрированному слушателю (UI / UITextInput)
            if (m_uiActive && !isConsoleActive() && m_keyCallback) {
                m_keyCallback(key, action);
            }
        }

        void Input::onMouseButton(int button, int action) {
            KeyCode code = glfwButtonToKeyCode(button);
            if (code == KeyCode::None) return;

            size_t index = static_cast<size_t>(code);

            if (action == GLFW_PRESS) {
                m_keysHeld[index] = true;
                m_keysPressed[index] = true;
            }
            else if (action == GLFW_RELEASE) {
                m_keysHeld[index] = false;
                m_keysReleased[index] = true;
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

        void Input::onScroll(double yoffset) {
            if (m_console && m_console->isVisible()) {
                m_console->onScroll(static_cast<float>(yoffset));
                return;
            }
            m_scrollY += static_cast<float>(yoffset);
        }

        void Input::onChar(unsigned int codepoint) {
            if (m_console && m_console->isVisible()) {
                m_console->onChar(codepoint);
                return;
            }

            // Передаём введённый символ в UI
            if (m_uiActive && m_charCallback) {
                m_charCallback(codepoint);
            }
        }

        bool Input::isConsoleActive() const {
            return m_console && m_console->isVisible();
        }

        void Input::setUIActive(bool active) {
            m_uiActive = active;
            if (active) m_firstMouse = true;
        }

    }
}