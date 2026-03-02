#pragma once
#include <memory>
#include "UIRenderer.h"
#include "UIFont.h"
#include "UIWidget.h"

class UIManager {
public:
    UIManager();
    ~UIManager();

    void init();
    void render();
    void onResize(int width, int height);
    void onMouseMove(float x, float y);
    void onMouseButton(float x, float y, int button, bool pressed);

    UIFont* getFont() const { return m_font.get(); }
    UIRenderer* getRenderer() const { return m_renderer.get(); } // Добавлено

private:
    void createFont();

    std::unique_ptr<UIRenderer> m_renderer;
    std::unique_ptr<UIFont> m_font;
    std::shared_ptr<UIPanel> m_rootPanel;
    bool m_initialized = false;
};