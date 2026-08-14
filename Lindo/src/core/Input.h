#pragma once
#include <glm/glm.hpp>

class Window;

class Input {
public:
    void update(Window* window); // вызывается каждый кадр для сброса одноразовых флагов

    // Состояние клавиш (удержание)
    bool isForwardPressed() const { return m_forward; }
    bool isBackwardPressed() const { return m_backward; }
    bool isLeftPressed() const { return m_left; }
    bool isRightPressed() const { return m_right; }
    bool isJumpPressed() const { return m_jump; }
    bool isCrouchPressed() const { return m_crouch; }
    bool isF3Pressed() const { return m_f3Pressed; }
    bool isF11Pressed() const { return m_f11Pressed; }
    bool isEscapePressed() const { return m_escapePressed; }

    // Для обработки однократных нажатий (потребляются после проверки)
    bool consumeF3();
    bool consumeF11();
    bool consumeEscape();

    // Мышь
    glm::vec2 getMouseDelta() const { return m_mouseDelta; }
    float getScrollY() const { return m_scrollY; }
    void resetMouseDelta() { m_mouseDelta = glm::vec2(0.0f); }

    // Для колбэков
    void onKey(int key, int action);
    void onMouseMove(double x, double y);
    void onMouseButton(int button, int action);
    void onScroll(double yoffset);

    // Состояние UI
    bool isUIActive() const { return m_uiActive; }
    void setUIActive(bool active);

private:
    // Флаги клавиш
    bool m_forward = false, m_backward = false, m_left = false, m_right = false;
    bool m_jump = false, m_crouch = false;
    bool m_f3Pressed = false, m_f11Pressed = false, m_escapePressed = false;

    // Для однократных нажатий
    bool m_f3Consumed = true, m_f11Consumed = true, m_escapeConsumed = true;

    // Мышь
    double m_lastX = 0.0, m_lastY = 0.0;
    bool m_firstMouse = true;
    glm::vec2 m_mouseDelta = glm::vec2(0.0f);
    float m_scrollY = 0.0f;

    // Флаг активности UI (курсор включен/выключен)
    bool m_uiActive = false;
};