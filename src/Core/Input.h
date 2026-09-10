#pragma once
#include <glm/glm.hpp>
#include <Core/OGL.h>
#include <string>
#include <unordered_map>
#include <vector>
#include <functional>
#include <Platform/Input/KeyCode.h>

namespace Lindo {
    class Window;
}

namespace Lindo {
    namespace Debug {
        class Console;
    }
}

namespace Lindo {
    namespace Input {
        /**
         * @brief Менеджер ввода в стиле Unity (GetKey, GetKeyDown, GetKeyUp).
         */
        class Input {
        public:
            using CharCallback = std::function<void(unsigned int)>;
            using KeyCallback = std::function<void(int key, int action)>;

            static Input& Get() { return *s_Instance; }

            Input();
            ~Input() = default;

            /**
             * @brief Обновляет состояния фаз клавиш (Down/Up) в конце кадра.
             * @param window Указатель на окно для смены режима курсора.
             */
            void update(Lindo::Window* window);

            // --- API ВВОДА (Unity Style) ---

            bool getKey(KeyCode key) const;
            bool getKeyDown(KeyCode key) const;
            bool getKeyUp(KeyCode key) const;

            // Backward-compatible GLFW-style overloads for legacy engine code.
            bool getKey(int glfwKey) const;
            bool getKeyDown(int glfwKey) const;
            bool getKeyUp(int glfwKey) const;

            bool getMouseButton(int button) const;
            bool getMouseButtonDown(int button) const;
            bool getMouseButtonUp(int button) const;

            bool getKey(const std::string& name) const;
            bool getKeyDown(const std::string& name) const;
            bool getKeyUp(const std::string& name) const;

            float getAxis(const std::string& positive, const std::string& negative) const;
            bool getAction(const std::string& action) const { return getKey(action); }
            bool getActionDown(const std::string& action) const { return getKeyDown(action); }
            bool getActionUp(const std::string& action) const { return getKeyUp(action); }

            bool consumeKey(KeyCode key);
            bool consumeKey(int glfwKey);
            bool consumeF3() { return consumeKey(KeyCode::F3); }
            bool consumeF4() { return consumeKey(KeyCode::F4); }
            bool consumeF11() { return consumeKey(KeyCode::F11); }
            bool consumeEscape() { return consumeKey(KeyCode::Escape); }

            // Алиас для кнопок (GetButton в Unity)
            bool getButton(const std::string& actionName) const { return getKey(actionName); }
            bool getButtonDown(const std::string& actionName) const { return getKeyDown(actionName); }
            bool getButtonUp(const std::string& actionName) const { return getKeyUp(actionName); }

            /**
             * @brief Привязывает строковый алиас к клавише (например, "Jump" -> KeyCode::Space).
             */
            void bindAction(const std::string& actionName, KeyCode key);

            // --- МЫШЬ И СРОЛЛ ---

            glm::vec2 getMouseDelta() const { return m_mouseDelta; }
            float getMouseDeltaX() const { return m_mouseDelta.x; }
            float getMouseDeltaY() const { return m_mouseDelta.y; }
            float getScrollY() const { return m_scrollY; }
            void resetMouseDelta() { m_mouseDelta = glm::vec2(0.0f); }

            // --- СИСТЕМНЫЕ КОЛБЭКИ (GLFW) ---

            void onKey(int key, int action);
            void onChar(unsigned int codepoint);
            void onMouseMove(double x, double y);
            void onMouseButton(int button, int action);
            void onScroll(double yoffset);

            // --- ПОДПИСКА НА СОБЫТИЯ ВВОДА ТЕКСТА И КЛАВИШ ---
            void setCharCallback(CharCallback callback) { m_charCallback = callback; }
            void setKeyCallback(KeyCallback callback) { m_keyCallback = callback; }

            // --- СОСТОЯНИЯ И КОНСОЛЬ ---

            bool isUIActive() const { return m_uiActive; }
            void setUIActive(bool active);
            void setConsole(Lindo::Debug::Console* console) { m_console = console; }
            bool isConsoleActive() const;

        private:
            void initKeyMappings();
            KeyCode glfwKeyToKeyCode(int glfwKey) const;
            KeyCode glfwButtonToKeyCode(int glfwButton) const;

        private:
            static Input* s_Instance;

            // Массивы фазовых состояний клавиш
            static constexpr size_t KEY_COUNT = static_cast<size_t>(KeyCode::Count);
            bool m_keysHeld[KEY_COUNT] = { false };
            bool m_keysPressed[KEY_COUNT] = { false };
            bool m_keysReleased[KEY_COUNT] = { false };

            // Карты трансляции строк и кодов GLFW
            std::unordered_map<std::string, KeyCode> m_stringToKeyMap;
            std::unordered_map<int, KeyCode> m_glfwToKeyMap;

            // Колбэки для передачи символов и спец-клавиш UI элементам
            CharCallback m_charCallback = nullptr;
            KeyCallback m_keyCallback = nullptr;

            // Мышь
            double m_lastX = 0.0;
            double m_lastY = 0.0;
            bool m_firstMouse = true;
            glm::vec2 m_mouseDelta = glm::vec2(0.0f);
            float m_scrollY = 0.0f;

            // UI и Консоль
            bool m_uiActive = false;
            Lindo::Debug::Console* m_console = nullptr;
        };

    }
}