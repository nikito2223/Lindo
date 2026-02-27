#pragma once
#include "UIRenderer.h"
#include "UIFont.h"
#include <functional>
#include <memory>
#include <vector>
#include <string>

class UIWidget {
public:
    UIWidget() = default;
    virtual ~UIWidget() = default;

    virtual void update(float dt) {}
    virtual void render(UIRenderer& renderer, UIFont* font) = 0;
    virtual bool onMouseMove(float x, float y) { return false; }
    virtual bool onMouseButton(float x, float y, int button, bool down) { return false; }

    virtual void setTextSize(float size) { m_textSize = size; }
    virtual float getTextSize() const { return m_textSize; }

    void setPosition(float x, float y) { m_rect.x = x; m_rect.y = y; }
    void setSize(float w, float h) { m_rect.w = w; m_rect.h = h; }
    Rect getRect() const { return m_rect; }

protected:
    Rect m_rect;
    float m_textSize = 24.0f; // размер текста по умолчанию
};

// Простая кнопка
class UIButton : public UIWidget {
public:
    UIButton(const std::string& label, std::function<void()> onClick);

    void setNormalColor(const Color& c) { m_normalColor = c; }
    void setHoveredColor(const Color& c) { m_hoveredColor = c; }
    void setPressedColor(const Color& c) { m_pressedColor = c; }
    void setTextColor(const Color& c) { m_textColor = c; }

    void render(UIRenderer& renderer, UIFont* font) override;
    bool onMouseMove(float x, float y) override;
    bool onMouseButton(float x, float y, int button, bool down) override;

    void setTextSize(float size) override { m_textSize = size; }
    void setLabel(const std::string& label) { m_label = label; }

private:
    std::string m_label;
    std::function<void()> m_onClick;
    bool m_hovered = false;
    bool m_pressed = false;

    Color m_normalColor{ 0.7f, 0.7f, 0.7f, 1.0f };
    Color m_hoveredColor{ 0.8f, 0.8f, 0.8f, 1.0f };
    Color m_pressedColor{ 0.5f, 0.5f, 0.5f, 1.0f };
    Color m_textColor{ 1.0f, 1.0f, 1.0f, 1.0f }; // цвет текста по умолчанию
};

// Панель-контейнер
class UIPanel : public UIWidget {
public:
    UIPanel() = default;

    void addChild(std::shared_ptr<UIWidget> child);
    void render(UIRenderer& renderer, UIFont* font) override;
    bool onMouseMove(float x, float y) override;
    bool onMouseButton(float x, float y, int button, bool down) override;

private:
    std::vector<std::shared_ptr<UIWidget>> m_children;
};

// Текстовое поле
class UILabel : public UIWidget {
public:
    UILabel(const std::string& text);
    void render(UIRenderer& renderer, UIFont* font) override;
    void setText(const std::string& text) { m_text = text; }
    void setTextSize(float size) override { m_textSize = size; }

private:
    std::string m_text;
};

// Изображение
class UIImage : public UIWidget {
public:
    UIImage(unsigned int textureId);
    void render(UIRenderer& renderer, UIFont* font) override;

private:
    unsigned int m_textureId;
};