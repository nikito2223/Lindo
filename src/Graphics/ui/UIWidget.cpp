#include "UIWidget.h"
#include <algorithm>
#include "Core/Time/Time.h"

namespace Lindo {
    namespace Graphics {
        namespace UI {

            // ================================================================
            // UIButton
            // ================================================================

            UIButton::UIButton(const std::string& label, std::function<void()> onClick)
                : m_label(label), m_onClick(onClick) {
            }

            void UIButton::render(UIRenderer& renderer, UIFont* font) {
                if (!m_visible || !font) return;

                Color bgColor = m_pressed ? m_pressedColor : (m_hovered ? m_hoveredColor : m_normalColor);
                renderer.drawRect(m_rect, bgColor);

                // Рамка
                renderer.drawRect(Rect(m_rect.x, m_rect.y, m_rect.w, 1), Color(0, 0, 0));
                renderer.drawRect(Rect(m_rect.x, m_rect.y + m_rect.h - 1, m_rect.w, 1), Color(0, 0, 0));
                renderer.drawRect(Rect(m_rect.x, m_rect.y, 1, m_rect.h), Color(0, 0, 0));
                renderer.drawRect(Rect(m_rect.x + m_rect.w - 1, m_rect.y, 1, m_rect.h), Color(0, 0, 0));

                if (!m_label.empty()) {
                    float scale = m_textSize / font->getLineHeight();
                    float textWidth = font->getStringWidthWithSize(m_label, m_textSize);
                    float textHeight = (font->getAscent() - font->getDescent()) * scale;

                    float x = m_rect.x + (m_rect.w - textWidth) * 0.5f;
                    float top = m_rect.y + (m_rect.h - textHeight) * 0.5f;
                    float baselineY = top + font->getAscent() * scale;

                    Color textColor = m_pressed
                        ? Color(m_textColor.r * 0.7f, m_textColor.g * 0.7f, m_textColor.b * 0.7f, m_textColor.a)
                        : m_textColor;

                    // ✅ ВЫДЕЛЯЕМ ПАМЯТЬ ЗАРАНЕЕ - 6 вершин на символ (2 треугольника)
                    std::vector<UIRenderer::Vertex> vertices;
                    vertices.reserve(m_label.size() * 6); // предотвращает множество переаллокаций

                    font->getTextVerticesWithSize(m_label, x, baselineY, m_textSize, textColor, vertices);
                    if (!vertices.empty()) {
                        renderer.drawRaw(vertices, font->getTextureID());
                    }
                }
            }

            bool UIButton::onMouseMove(float x, float y) {
                if (!m_visible) return false;
                m_hovered = m_rect.contains(x, y);
                return m_hovered;
            }

            bool UIButton::onMouseButton(float x, float y, int button, bool down) {
                if (!m_visible || button != 0) return false;

                bool inside = m_rect.contains(x, y);
                if (down) {
                    if (inside) { m_pressed = true; return true; }
                }
                else {
                    if (m_pressed) {
                        m_pressed = false;
                        if (inside && m_onClick) m_onClick();
                        return true;
                    }
                }
                return false;
            }

            // ================================================================
            // UIPanel
            // ================================================================

            void UIPanel::addChild(std::shared_ptr<UIWidget> child) {
                if (child) {
                    m_children.push_back(child);
                    m_sortDirty = true;
                }
            }

            void UIPanel::removeChild(const std::shared_ptr<UIWidget>& child) {
                auto it = std::find(m_children.begin(), m_children.end(), child);
                if (it != m_children.end()) {
                    m_children.erase(it);
                    m_sortDirty = true;
                }
            }

            void UIPanel::setChildZOrder(const std::shared_ptr<UIWidget>& child, int z) {
                child->setZOrder(z);
                m_sortDirty = true;
            }

            void UIPanel::rebuildSortedList() {
                m_sortedChildren.clear();
                m_sortedChildren.reserve(m_children.size());
                for (auto& c : m_children) {
                    m_sortedChildren.push_back(c.get());
                }
                // stable_sort: при равном zOrder сохраняется порядок добавления
                std::stable_sort(m_sortedChildren.begin(), m_sortedChildren.end(),
                    [](const UIWidget* a, const UIWidget* b) {
                        return a->getZOrder() < b->getZOrder();
                    });
                m_sortDirty = false;
            }

            void UIPanel::render(UIRenderer& renderer, UIFont* font) {
                if (!m_visible) return;

                renderer.drawRect(m_rect, m_panelColor);

                if (m_sortDirty) rebuildSortedList();

                // Рендерим от низкого Z к высокому: нижние слои рисуются первыми
                for (UIWidget* child : m_sortedChildren) {
                    if (child->isVisible()) {
                        child->render(renderer, font);
                    }
                }
            }

            bool UIPanel::onMouseMove(float x, float y) {
                if (!m_visible) return false;
                if (m_sortDirty) rebuildSortedList();

                // Ввод — обратный порядок: высокий Z перехватывает первым
                for (auto it = m_sortedChildren.rbegin(); it != m_sortedChildren.rend(); ++it) {
                    UIWidget* child = *it;
                    if (child->isVisible() && child->onMouseMove(x, y)) {
                        return true;
                    }
                }
                return false;
            }

            bool UIPanel::onMouseButton(float x, float y, int button, bool down) {
                if (!m_visible) return false;
                if (m_sortDirty) rebuildSortedList();

                for (auto it = m_sortedChildren.rbegin(); it != m_sortedChildren.rend(); ++it) {
                    UIWidget* child = *it;
                    if (child->isVisible() && child->onMouseButton(x, y, button, down)) {
                        return true;
                    }
                }
                return false;
            }

            // ================================================================
            // UILabel
            // ================================================================

            UILabel::UILabel(const std::string& text) : m_text(text) {}

            void UILabel::render(UIRenderer& renderer, UIFont* font) {
                if (!m_visible || !font || m_text.empty()) return;
            
                float scale = m_textSize / font->getLineHeight();
                // Корректный расчет baselineY для orthographic projection (Top-to-Bottom Y)
                float baselineY = m_rect.y + font->getAscent() * scale;
            
                std::vector<UIRenderer::Vertex> vertices;
                vertices.reserve(m_text.size() * 6); 
            
                font->getTextVerticesWithSize(m_text, m_rect.x, baselineY, m_textSize, m_textColor, vertices);
                if (!vertices.empty()) {
                    renderer.drawRaw(vertices, font->getTextureID());
                }
            }

            // ================================================================
            // UIImage
            // ================================================================

            UIImage::UIImage(unsigned int textureId) : m_textureId(textureId) {}

            void UIImage::render(UIRenderer& renderer, UIFont* font) {
                if (!m_visible) return;
                // Флипаем Y (y: 1 → 0) чтобы изображение не было перевёрнуто
                renderer.drawTexturedRect(m_rect, Rect(0.0f, 1.0f, 1.0f, -1.0f), m_textureId, Color(1, 1, 1, 1));
            }
            // ================================================================
            // UITextInput
            // ================================================================
                    
            UITextInput::UITextInput(const std::string& placeholder)
                : m_placeholder(placeholder) {}
                    
            void UITextInput::render(UIRenderer& renderer, UIFont* font) {
                if (!m_visible || !font) return;
            
                // Фон
                renderer.drawRect(m_rect, m_bgColor);
            
                // Рамка (при фокусе подсвечивается)
                Color border = m_focused ? m_focusedBorderColor : m_borderColor;
                renderer.drawRect(Rect(m_rect.x, m_rect.y, m_rect.w, 1), border);
                renderer.drawRect(Rect(m_rect.x, m_rect.y + m_rect.h - 1, m_rect.w, 1), border);
                renderer.drawRect(Rect(m_rect.x, m_rect.y, 1, m_rect.h), border);
                renderer.drawRect(Rect(m_rect.x + m_rect.w - 1, m_rect.y, 1, m_rect.h), border);
            
                std::string displayText = m_text.empty() ? m_placeholder : m_text;
                Color textColor = m_text.empty() ? m_placeholderColor : m_textColor;
            
                if (!displayText.empty()) {
                    float scale = m_textSize / font->getLineHeight();
                    float textHeight = (font->getAscent() - font->getDescent()) * scale;
                    float paddingX = 10.0f;
                    float x = m_rect.x + paddingX;
                    float baselineY = m_rect.y + (m_rect.h - textHeight) * 0.5f + font->getAscent() * scale;
                
                    std::vector<UIRenderer::Vertex> vertices;
                    vertices.reserve(displayText.size() * 6);
                    font->getTextVerticesWithSize(displayText, x, baselineY, m_textSize, textColor, vertices);
                    
                    if (!vertices.empty()) {
                        renderer.drawRaw(vertices, font->getTextureID());
                    }
                }
            
                // Отрисовка курсора '|' если элемент в фокусе
                if (m_focused) {
                    float scale = m_textSize / font->getLineHeight();
                    float textW = m_text.empty() ? 0.0f : font->getStringWidthWithSize(m_text, m_textSize);
                    float cursorX = m_rect.x + 10.0f + textW;
                    float cursorY = m_rect.y + 6.0f;
                    float cursorH = m_rect.h - 12.0f;
                
                    renderer.drawRect(Rect(cursorX, cursorY, 2.0f, cursorH), m_focusedBorderColor);
                }
            }
            
            bool UITextInput::onMouseMove(float x, float y) {
                if (!m_visible) return false;
                m_hovered = m_rect.contains(x, y);
                return m_hovered;
            }
            
            bool UITextInput::onMouseButton(float x, float y, int button, bool down) {
                if (!m_visible || button != 0) return false;
                if (down) {
                    m_focused = m_rect.contains(x, y);
                    return m_focused;
                }
                return false;
            }

            void UITextInput::selectAll() {
                // Реализация выделения (при необходимости)
            }

            void UITextInput::copy() {
                // Реализация копирования в буфер обмена
            }

            void UITextInput::paste() {
                // Реализация вставки из буфера обмена
            }

            void UITextInput::cut() {
                // Реализация вырезания текста
            }

            void UITextInput::onBackspace() {
                if (!m_text.empty()) {
                    while (!m_text.empty()) {
                        char c = m_text.back();
                        m_text.pop_back();
                        // Удаляем байты UTF-8 символа
                        if ((c & 0xC0) != 0x80) break; 
                    }
                }
            }

            void UITextInput::onDeleteForward() {
                // Реализация удаления символа перед курсором (Delete)
            }

            void UITextInput::moveCursorToStart() {
                // Перемещение курсора в начало строки
            }

            void UITextInput::moveCursorToEnd() {
                // Перемещение курсора в конец строки
            }

            void UITextInput::moveCursorLeft() {
                // Смещение курсора влево
            }

            void UITextInput::moveCursorRight() {
                // Смещение курсора вправо
            }
            
            bool UITextInput::onChar(unsigned int codepoint) {
                if (!m_focused || !m_visible) return false;
                if (codepoint >= 32) {
                    // Поддержка UTF-8 символов
                    if (codepoint <= 0x7F) {
                        m_text += static_cast<char>(codepoint);
                    } else if (codepoint <= 0x7FF) {
                        m_text += static_cast<char>(0xC0 | (codepoint >> 6));
                        m_text += static_cast<char>(0x80 | (codepoint & 0x3F));
                    } else if (codepoint <= 0xFFFF) {
                        m_text += static_cast<char>(0xE0 | (codepoint >> 12));
                        m_text += static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F));
                        m_text += static_cast<char>(0x80 | (codepoint & 0x3F));
                    }
                    return true;
                }
                return false;
            }
            
            bool UITextInput::onKey(int key, int scancode, int action, int mods) {
                if (!m_focused || !m_visible || (action != 1 && action != 2)) return false;
            
                // Backspace (удаление последнего UTF-8 символа)
                if (key == 259 && !m_text.empty()) { 
                    while (!m_text.empty()) {
                        char c = m_text.back();
                        m_text.pop_back();
                        if ((c & 0xC0) != 0x80) break; // Стираем базовая последовательность байт
                    }
                    return true;
                }
                // Enter (Подтверждение ввода)
                if (key == 257) { 
                    if (m_onSubmit) m_onSubmit(m_text);
                    return true;
                }
                return false;
            }
            
            // ================================================================
            // UISlider
            // ================================================================
            
            UISlider::UISlider(float minVal, float maxVal, float currentVal)
                : m_min(minVal), m_max(maxVal), m_value(currentVal) {}
            
            void UISlider::setValue(float val) {
                m_value = std::clamp(val, m_min, m_max);
                if (m_onChange) m_onChange(m_value);
            }
            
            void UISlider::setRange(float minVal, float maxVal) {
                m_min = minVal;
                m_max = maxVal;
                setValue(m_value);
            }
            
            void UISlider::render(UIRenderer& renderer, UIFont* font) {
                if (!m_visible) return;
            
                // Отрисовка дорожки (Track)
                float trackHeight = 6.0f;
                float trackY = m_rect.y + (m_rect.h - trackHeight) * 0.5f;
                renderer.drawRect(Rect(m_rect.x, trackY, m_rect.w, trackHeight), m_trackColor);
            
                // Заполнение дорожки слева направо (Fill)
                float normalized = (m_max > m_min) ? (m_value - m_min) / (m_max - m_min) : 0.0f;
                float fillWidth = m_rect.w * normalized;
                renderer.drawRect(Rect(m_rect.x, trackY, fillWidth, trackHeight), m_fillColor);
            
                // Ползунок (Thumb)
                float thumbWidth = 14.0f;
                float thumbHeight = m_rect.h - 4.0f;
                float thumbX = m_rect.x + fillWidth - (thumbWidth * 0.5f);
                thumbX = std::clamp(thumbX, m_rect.x, m_rect.x + m_rect.w - thumbWidth);
                float thumbY = m_rect.y + 2.0f;
            
                renderer.drawRect(Rect(thumbX, thumbY, thumbWidth, thumbHeight), m_thumbColor);
            }
            
            bool UISlider::onMouseMove(float x, float y) {
                if (!m_visible) return false;
                if (m_dragging) {
                    float relX = std::clamp(x - m_rect.x, 0.0f, m_rect.w);
                    float pct = relX / m_rect.w;
                    setValue(m_min + pct * (m_max - m_min));
                    return true;
                }
                return m_rect.contains(x, y);
            }
            
            bool UISlider::onMouseButton(float x, float y, int button, bool down) {
                if (!m_visible || button != 0) return false;
            
                if (down) {
                    if (m_rect.contains(x, y)) {
                        m_dragging = true;
                        float relX = std::clamp(x - m_rect.x, 0.0f, m_rect.w);
                        float pct = relX / m_rect.w;
                        setValue(m_min + pct * (m_max - m_min));
                        return true;
                    }
                } else {
                    if (m_dragging) {
                        m_dragging = false;
                        return true;
                    }
                }
                return false;
            }

            // ================================================================
            // UIDropDown
            // ================================================================
                    
            UIDropDown::UIDropDown(const std::vector<std::string>& options) 
                : m_options(options) {
                if (!m_options.empty()) m_selectedIndex = 0;
            }
            
            void UIDropDown::setOptions(const std::vector<std::string>& options) {
                m_options = options;
                m_selectedIndex = m_options.empty() ? -1 : 0;
            }
            
            void UIDropDown::setSelectedIndex(int index) {
                if (index >= 0 && index < static_cast<int>(m_options.size())) {
                    m_selectedIndex = index;
                    if (m_onSelect) m_onSelect(m_selectedIndex, m_options[m_selectedIndex]);
                }
            }
            
            std::string UIDropDown::getSelectedOption() const {
                if (m_selectedIndex >= 0 && m_selectedIndex < static_cast<int>(m_options.size())) {
                    return m_options[m_selectedIndex];
                }
                return "";
            }
            
            void UIDropDown::render(UIRenderer& renderer, UIFont* font) {
                if (!m_visible || !font) return;
            
                // Главное окно списка
                renderer.drawRect(m_rect, m_expanded ? m_hoverColor : m_mainColor);
            
                // Стрелочка справа (V)
                std::string arrow = m_expanded ? "^" : "v";
                float scale = m_textSize / font->getLineHeight();
                float baselineY = m_rect.y + (m_rect.h - (font->getAscent() - font->getDescent()) * scale) * 0.5f + font->getAscent() * scale;
            
                // Выбранный текст
                std::string selectedText = getSelectedOption();
                if (!selectedText.empty()) {
                    std::vector<UIRenderer::Vertex> vertices;
                    font->getTextVerticesWithSize(selectedText, m_rect.x + 10.0f, baselineY, m_textSize, m_textColor, vertices);
                    if (!vertices.empty()) renderer.drawRaw(vertices, font->getTextureID());
                }
            
                // Текст стрелочки
                std::vector<UIRenderer::Vertex> arrowVerts;
                font->getTextVerticesWithSize(arrow, m_rect.x + m_rect.w - 20.0f, baselineY, m_textSize, m_textColor, arrowVerts);
                if (!arrowVerts.empty()) renderer.drawRaw(arrowVerts, font->getTextureID());
            
                // Выпадающее меню
                if (m_expanded) {
                    float itemH = m_rect.h;
                    Rect dropRect(m_rect.x, m_rect.y + m_rect.h, m_rect.w, itemH * m_options.size());
                    renderer.drawRect(dropRect, m_dropdownColor);
                
                    for (size_t i = 0; i < m_options.size(); ++i) {
                        Rect itemR(m_rect.x, m_rect.y + m_rect.h + i * itemH, m_rect.w, itemH);
                        if (static_cast<int>(i) == m_hoveredIndex) {
                            renderer.drawRect(itemR, m_hoverColor);
                        }
                    
                        float itemBaseline = itemR.y + (itemR.h - (font->getAscent() - font->getDescent()) * scale) * 0.5f + font->getAscent() * scale;
                        std::vector<UIRenderer::Vertex> itemVerts;
                        font->getTextVerticesWithSize(m_options[i], itemR.x + 10.0f, itemBaseline, m_textSize, m_textColor, itemVerts);
                        if (!itemVerts.empty()) renderer.drawRaw(itemVerts, font->getTextureID());
                    }
                }
            }
            
            bool UIDropDown::onMouseMove(float x, float y) {
                if (!m_visible) return false;
            
                if (m_expanded) {
                    float itemH = m_rect.h;
                    m_hoveredIndex = -1;
                    for (size_t i = 0; i < m_options.size(); ++i) {
                        Rect itemR(m_rect.x, m_rect.y + m_rect.h + i * itemH, m_rect.w, itemH);
                        if (itemR.contains(x, y)) {
                            m_hoveredIndex = static_cast<int>(i);
                            return true;
                        }
                    }
                }
            
                return m_rect.contains(x, y);
            }
            
            bool UIDropDown::onMouseButton(float x, float y, int button, bool down) {
                if (!m_visible || button != 0) return false;
            
                if (down) {
                    if (m_rect.contains(x, y)) {
                        m_expanded = !m_expanded;
                        return true;
                    }
                
                    if (m_expanded) {
                        float itemH = m_rect.h;
                        for (size_t i = 0; i < m_options.size(); ++i) {
                            Rect itemR(m_rect.x, m_rect.y + m_rect.h + i * itemH, m_rect.w, itemH);
                            if (itemR.contains(x, y)) {
                                setSelectedIndex(static_cast<int>(i));
                                m_expanded = false;
                                return true;
                            }
                        }
                        m_expanded = false; // Закрыть при клике мимо
                    }
                }
                return false;
            }

            // ================================================================
            // UIToggle
            // ================================================================
                    
            UIToggle::UIToggle(bool checked, std::function<void(bool)> onChange)
                : m_checked(checked), m_onChange(onChange) {}
                    
            void UIToggle::setChecked(bool checked) {
                if (m_checked != checked) {
                    m_checked = checked;
                    if (m_onChange) m_onChange(m_checked);
                }
            }
            
            void UIToggle::render(UIRenderer& renderer, UIFont* font) {
                if (!m_visible) return;
            
                // 1. Отрисовка коробки переключателя (Box)
                float boxSize = std::min(m_rect.w, m_rect.h);
                Rect boxRect(m_rect.x, m_rect.y + (m_rect.h - boxSize) * 0.5f, boxSize, boxSize);
            
                Color currentBg = m_hovered ? m_hoverColor : m_bgColor;
                renderer.drawRect(boxRect, currentBg);
            
                // Рамка вокруг коробки
                Color border = m_hovered ? Color(0.4f, 0.4f, 0.5f, 1.0f) : Color(0.25f, 0.25f, 0.3f, 1.0f);
                renderer.drawRect(Rect(boxRect.x, boxRect.y, boxRect.w, 1), border);
                renderer.drawRect(Rect(boxRect.x, boxRect.y + boxRect.h - 1, boxRect.w, 1), border);
                renderer.drawRect(Rect(boxRect.x, boxRect.y, 1, boxRect.h), border);
                renderer.drawRect(Rect(boxRect.x + boxRect.w - 1, boxRect.y, 1, boxRect.h), border);
            
                // 2. Отрисовка состояния (галочка/заливка)
                if (m_checked) {
                    float padding = boxSize * 0.2f;
                    Rect checkInner(boxRect.x + padding, boxRect.y + padding, boxSize - padding * 2.0f, boxSize - padding * 2.0f);
                    renderer.drawRect(checkInner, m_checkColor);
                }
            
                // 3. Текст метки справа от переключателя
                if (!m_label.empty() && font) {
                    float scale = m_textSize / font->getLineHeight();
                    float textX = boxRect.x + boxSize + 10.0f;
                    float textHeight = (font->getAscent() - font->getDescent()) * scale;
                    float baselineY = m_rect.y + (m_rect.h - textHeight) * 0.5f + font->getAscent() * scale;
                
                    std::vector<UIRenderer::Vertex> vertices;
                    vertices.reserve(m_label.size() * 6);
                    font->getTextVerticesWithSize(m_label, textX, baselineY, m_textSize, m_textColor, vertices);
                    
                    if (!vertices.empty()) {
                        renderer.drawRaw(vertices, font->getTextureID());
                    }
                }
            }
            
            bool UIToggle::onMouseMove(float x, float y) {
                if (!m_visible) return false;
                m_hovered = m_rect.contains(x, y);
                return m_hovered;
            }
            
            bool UIToggle::onMouseButton(float x, float y, int button, bool down) {
                if (!m_visible || button != 0) return false;
            
                if (down && m_rect.contains(x, y)) {
                    setChecked(!m_checked);
                    return true;
                }
                return false;
            }
        }
    }
}