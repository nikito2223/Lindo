#include "UIWidget.h"

UIButton::UIButton(const std::string& label, std::function<void()> onClick)
    : m_label(label), m_onClick(onClick) {}



void UIButton::render(UIRenderer& renderer, UIFont* font) {
    if (!font) return;

    // Выбираем цвет фона в зависимости от состояния
    Color bgColor = m_pressed ? m_pressedColor : (m_hovered ? m_hoveredColor : m_normalColor);
    renderer.drawRect(m_rect, bgColor);

    // Рисуем рамку
    renderer.drawRect(Rect(m_rect.x, m_rect.y, m_rect.w, 1), Color(0, 0, 0));
    renderer.drawRect(Rect(m_rect.x, m_rect.y + m_rect.h - 1, m_rect.w, 1), Color(0, 0, 0));
    renderer.drawRect(Rect(m_rect.x, m_rect.y, 1, m_rect.h), Color(0, 0, 0));
    renderer.drawRect(Rect(m_rect.x + m_rect.w - 1, m_rect.y, 1, m_rect.h), Color(0, 0, 0));

    // Рендерим текст
    if (!m_label.empty()) {
        // Вычисляем масштаб относительно базового размера шрифта (24)
        float scale = m_textSize / font->getLineHeight();

        // Ширина текста с учётом масштаба
        float textWidth = font->getStringWidthWithSize(m_label, m_textSize);

        // Высота текста с учётом масштаба
        float textHeight = (font->getAscent() - font->getDescent()) * scale;

        // Позиция для центрирования
        float x = m_rect.x + (m_rect.w - textWidth) * 0.5f;
        float top = m_rect.y + (m_rect.h - textHeight) * 0.5f;
        float baselineY = top + font->getAscent() * scale;

        // Цвет текста (затемняем при нажатии)
        Color textColor = m_pressed ?
            Color(m_textColor.r * 0.7f, m_textColor.g * 0.7f, m_textColor.b * 0.7f, m_textColor.a) :
            m_textColor;

        std::vector<UIRenderer::Vertex> vertices;
        font->getTextVerticesWithSize(m_label, x, baselineY, m_textSize, textColor, vertices);

        if (!vertices.empty()) {
            renderer.drawRaw(vertices, font->getTextureID());
        }
    }
}


bool UIButton::onMouseMove(float x, float y) {
    bool inside = m_rect.contains(x, y);
    m_hovered = inside;
    return inside; // возвращаем, захвачено ли событие
}

bool UIButton::onMouseButton(float x, float y, int button, bool down)
{
    if (button != 0) return false; // левая кнопка

    bool inside = m_rect.contains(x, y);

    if (down)
    {
        if (inside)
        {
            m_pressed = true;
            return true;
        }
    }
    else
    {
        if (m_pressed)
        {
            if (inside && m_onClick)
                m_onClick();

            m_pressed = false;
            return true;
        }
    }

    return false;
}

void UIPanel::addChild(std::shared_ptr<UIWidget> child) {
    m_children.push_back(child);
}

void UIPanel::render(UIRenderer& renderer, UIFont* font) {
    // Можно нарисовать фон панели
    renderer.drawRect(m_rect, m_panelColor);
    for (auto& child : m_children) {
        child->render(renderer, font);
    }
}

bool UIPanel::onMouseMove(float x, float y) {
    // Сначала пробуем передать события детям в обратном порядке (чтобы верхний получал первым)
    for (auto it = m_children.rbegin(); it != m_children.rend(); ++it) {
        if ((*it)->onMouseMove(x, y)) {
            return true;
        }
    }
    return false;
}

bool UIPanel::onMouseButton(float x, float y, int button, bool down) {
    for (auto it = m_children.rbegin(); it != m_children.rend(); ++it) {
        if ((*it)->onMouseButton(x, y, button, down)) {
            return true;
        }
    }
    return false;
}

UILabel::UILabel(const std::string& text) : m_text(text) {}

void UILabel::render(UIRenderer& renderer, UIFont* font) {
    if (!font || m_text.empty()) return;

    // Вычисляем масштаб
    float scale = m_textSize / font->getLineHeight();

    // Позиция для текста (по умолчанию выравнивание по левому краю)
    float baselineY = m_rect.y + font->getAscent() * scale;

    std::vector<UIRenderer::Vertex> vertices;
    font->getTextVerticesWithSize(m_text, m_rect.x, baselineY, m_textSize, m_textColor, vertices);

    if (!vertices.empty()) {
        renderer.drawRaw(vertices, font->getTextureID());
    }
}


UIImage::UIImage(unsigned int textureId) : m_textureId(textureId) {}

void UIImage::render(UIRenderer& renderer, UIFont* font) {
    renderer.drawTexturedRect(m_rect, Rect(0, 0, 1, 1), m_textureId, Color(1, 1, 1, 1));
}