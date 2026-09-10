#pragma once
#include "UIRenderer.h"
#include "UIFont.h"
#include "world/managers/LayerManager.h"
#include <functional>
#include <memory>
#include <vector>
#include <string>
#include <algorithm>

namespace Lindo {
    namespace Graphics {
        namespace UI {

            // ====================================================================
            // UIWidget — базовый класс всех UI-элементов
            // Поддерживает Layer & Tag, а также Z-порядок рендеринга.
            // ====================================================================
            class UIWidget {
            public:
                UIWidget() {
                    // Автоматическая привязка к дефолтному UI слою из LayerManager
                    m_layer = World::LayerManager::get().getLayerByName("UI");
                }
                virtual ~UIWidget() = default;

                virtual void update() {}
                virtual void render(UIRenderer& renderer, UIFont* font) = 0;
                virtual bool onMouseMove(float x, float y) { return false; }
                virtual bool onMouseButton(float x, float y, int button, bool down) { return false; }

                virtual void setTextSize(float size) { m_textSize = size; }
                virtual float getTextSize() const { return m_textSize; }
                virtual bool onChar(unsigned int codepoint) { return false; }
                virtual bool onKey(int key, int scancode, int action, int mods) { return false; }

                void setPosition(float x, float y) { m_rect.x = x; m_rect.y = y; }
                void setSize(float w, float h) { m_rect.w = w; m_rect.h = h; }
                Rect getRect() const { return m_rect; }
                float getX() const { return m_rect.x; }
                float getY() const { return m_rect.y; }
                float getWidth() const { return m_rect.w; }
                float getHeight() const { return m_rect.h; }

                // ----------------------------------------------------------------
                // Свойства Layer & Tag
                // ----------------------------------------------------------------
                const std::string& getTag() const { return m_tag; }
                
                void setTag(const std::string& tag) {
                    World::LayerManager::get().registerTag(tag);
                    m_tag = tag;
                }

                bool compareTag(const std::string& tag) const {
                    return m_tag == tag;
                }

                uint8_t getLayer() const { return m_layer; }
                
                void setLayer(uint8_t layerIndex) {
                    if (layerIndex >= World::LayerManager::MAX_LAYERS) return;
                    m_layer = layerIndex;
                }

                void setLayerByName(const std::string& layerName) {
                    m_layer = World::LayerManager::get().getLayerByName(layerName);
                }

                std::string getLayerName() const {
                    return World::LayerManager::get().getLayerName(m_layer);
                }

                // ----------------------------------------------------------------
                // Z-порядок и Видимость
                // ----------------------------------------------------------------
                void setZOrder(int z) { m_zOrder = z; }
                int  getZOrder() const { return m_zOrder; }

                void setVisible(bool v) { m_visible = v; }
                bool isVisible() const { return m_visible; }

            protected:
                Rect        m_rect;
                float       m_textSize = 24.0f;
                int         m_zOrder = 0;
                bool        m_visible = true;

                std::string m_tag = "Untagged";
                uint8_t     m_layer = 4; // Индекс слоя по умолчанию (UI)
            };

            // ====================================================================
            // UIButton — кликабельная кнопка с текстом
            // ====================================================================
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
                Color m_textColor{ 1.0f, 1.0f, 1.0f, 1.0f };
            };

            // ====================================================================
            // UIPanel — контейнер с поддержкой Z-слоёв и дочерних поисков по Tag
            // ====================================================================
            class UIPanel : public UIWidget {
            public:
                UIPanel() = default;

                void addChild(std::shared_ptr<UIWidget> child);
                void removeChild(const std::shared_ptr<UIWidget>& child);
                void setChildZOrder(const std::shared_ptr<UIWidget>& child, int z);

                // Поиск потомков по тегу
                std::shared_ptr<UIWidget> findChildByTag(const std::string& tag) {
                    for (auto& child : m_children) {
                        if (child->compareTag(tag)) return child;
                    }
                    return nullptr;
                }

                void render(UIRenderer& renderer, UIFont* font) override;
                bool onMouseMove(float x, float y) override;
                bool onMouseButton(float x, float y, int button, bool down) override;
                void SetColor(Color color) { m_panelColor = color; }

                bool onChar(unsigned int codepoint) override {
                    if (!m_visible) return false;
                    for (auto child : m_children) {
                        if (child->isVisible() && child->onChar(codepoint)) return true;
                    }
                    return false;
                }
            
                bool onKey(int key, int scancode, int action, int mods) override {
                    if (!m_visible) return false;
                    for (auto child : m_children) {
                        if (child->isVisible() && child->onKey(key, scancode, action, mods)) return true;
                    }
                    return false;
                }

                size_t childCount() const { return m_children.size(); }
                void clearChildren() { m_children.clear(); m_sortedChildren.clear(); m_sortDirty = false; }

            private:
                void rebuildSortedList();

                std::vector<std::shared_ptr<UIWidget>> m_children;
                std::vector<UIWidget*>                  m_sortedChildren;
                bool  m_sortDirty = false;
                Color m_panelColor{ 1.0f, 1.0f, 1.0f, 1.0f };
            };

            // ====================================================================
            // UILabel — текстовая метка
            // ====================================================================
            class UILabel : public UIWidget {
            public:
                UILabel(const std::string& text);
                void render(UIRenderer& renderer, UIFont* font) override;
                void setText(const std::string& text) { m_text = text; }
                void setTextSize(float size) override { m_textSize = size; }
                void setTextColor(Color c) { m_textColor = c; }

            private:
                std::string m_text;
                Color m_textColor{ 1.0f, 1.0f, 1.0f, 1.0f };
            };

            // ====================================================================
            // UIDropDown — Выпадающий список
            // ====================================================================
            class UIDropDown : public UIWidget {
            public:
                UIDropDown(const std::vector<std::string>& options = {});
            
                void render(UIRenderer& renderer, UIFont* font) override;
                bool onMouseMove(float x, float y) override;
                bool onMouseButton(float x, float y, int button, bool down) override;
            
                void setOptions(const std::vector<std::string>& options);
                void setSelectedIndex(int index);
                int getSelectedIndex() const { return m_selectedIndex; }
                std::string getSelectedOption() const;
            
                void setOnSelect(std::function<void(int, const std::string&)> cb) { m_onSelect = cb; }
            
                void setMainColor(const Color& c) { m_mainColor = c; }
                void setHoverColor(const Color& c) { m_hoverColor = c; }
                void setDropdownColor(const Color& c) { m_dropdownColor = c; }
            
            private:
                std::vector<std::string> m_options;
                int m_selectedIndex = -1;
                int m_hoveredIndex = -1;
                bool m_expanded = false;
            
                Color m_mainColor{ 0.15f, 0.15f, 0.18f, 1.0f };
                Color m_hoverColor{ 0.25f, 0.25f, 0.3f, 1.0f };
                Color m_dropdownColor{ 0.12f, 0.12f, 0.14f, 0.98f };
                Color m_textColor{ 0.95f, 0.95f, 0.95f, 1.0f };
            
                std::function<void(int, const std::string&)> m_onSelect;
            };

            // ====================================================================
            // UIImage — текстурированный прямоугольник
            // ====================================================================
            class UIImage : public UIWidget {
            public:
                UIImage(unsigned int textureId);
                void render(UIRenderer& renderer, UIFont* font) override;

            private:
                unsigned int m_textureId;
            };

            // ====================================================================
            // UITextInput — Поле ввода текста с фокусом и поддержкой клавиатуры
            // ====================================================================
            class UITextInput : public UIWidget {
            public:
                UITextInput(const std::string& placeholder = "");
            
                void render(UIRenderer& renderer, UIFont* font) override;
                bool onMouseMove(float x, float y) override;
                bool onMouseButton(float x, float y, int button, bool down) override;
                bool onChar(unsigned int codepoint) override;
                bool onKey(int key, int scancode, int action, int mods) override;
            
                void setText(const std::string& text) { m_text = text; }
                const std::string& getText() const { return m_text; }
                void setPlaceholder(const std::string& ph) { m_placeholder = ph; }
                
                void setBackgroundColor(const Color& c) { m_bgColor = c; }
                void setFocusedBorderColor(const Color& c) { m_focusedBorderColor = c; }
                void setTextColor(const Color& c) { m_textColor = c; }
                
                void setFocused(bool focused) { m_focused = focused; }
                bool isFocused() const { return m_focused; }
            
                void setOnSubmit(std::function<void(const std::string&)> cb) { m_onSubmit = cb; }
                
                void selectAll();
                void copy();
                void paste();
                void cut();
                        
                void onBackspace();
                void onDeleteForward();
                        
                void moveCursorToStart();
                void moveCursorToEnd();
                void moveCursorLeft();
                void moveCursorRight();
                        
                void clearText() { m_text.clear(); }

            private:
                std::string m_text;
                std::string m_placeholder;
                bool m_focused = false;
                bool m_hovered = false;
            
                Color m_bgColor{ 0.1f, 0.1f, 0.12f, 0.9f };
                Color m_borderColor{ 0.25f, 0.25f, 0.3f, 1.0f };
                Color m_focusedBorderColor{ 0.2f, 0.6f, 0.9f, 1.0f };
                Color m_textColor{ 0.95f, 0.95f, 0.95f, 1.0f };
                Color m_placeholderColor{ 0.4f, 0.4f, 0.45f, 1.0f };
            
                std::function<void(const std::string&)> m_onSubmit;
            };
            
            // ====================================================================
            // UISlider — Ползунок диапазона значений
            // ====================================================================
            class UISlider : public UIWidget {
            public:
                UISlider(float minVal = 0.0f, float maxVal = 1.0f, float currentVal = 0.0f);
            
                void render(UIRenderer& renderer, UIFont* font) override;
                bool onMouseMove(float x, float y) override;
                bool onMouseButton(float x, float y, int button, bool down) override;
            
                void setValue(float val);
                float getValue() const { return m_value; }
            
                void setRange(float minVal, float maxVal);
                void setOnChange(std::function<void(float)> cb) { m_onChange = cb; }
            
                void setTrackColor(const Color& c) { m_trackColor = c; }
                void setFillColor(const Color& c) { m_fillColor = c; }
                void setThumbColor(const Color& c) { m_thumbColor = c; }
            
            private:
                float m_min = 0.0f;
                float m_max = 1.0f;
                float m_value = 0.0f;
                bool m_dragging = false;
            
                Color m_trackColor{ 0.15f, 0.15f, 0.18f, 1.0f };
                Color m_fillColor{ 0.2f, 0.6f, 0.8f, 1.0f };
                Color m_thumbColor{ 0.9f, 0.9f, 0.95f, 1.0f };
            
                std::function<void(float)> m_onChange;
            };

            // ====================================================================
            // UIToggle — Переключатель (Checkbox / Toggle Switch)
            // ====================================================================
            class UIToggle : public UIWidget {
            public:
                UIToggle(bool checked = false, std::function<void(bool)> onChange = nullptr);
            
                void render(UIRenderer& renderer, UIFont* font) override;
                bool onMouseMove(float x, float y) override;
                bool onMouseButton(float x, float y, int button, bool down) override;
            
                void setChecked(bool checked);
                bool isChecked() const { return m_checked; }
            
                void setLabel(const std::string& label) { m_label = label; }
                const std::string& getLabel() const { return m_label; }
            
                void setOnChange(std::function<void(bool)> cb) { m_onChange = cb; }
            
                void setBgColor(const Color& c) { m_bgColor = c; }
                void setCheckColor(const Color& c) { m_checkColor = c; }
                void setTextColor(const Color& c) { m_textColor = c; }
            
            private:
                bool m_checked = false;
                bool m_hovered = false;
                std::string m_label;
            
                Color m_bgColor{ 0.15f, 0.15f, 0.18f, 1.0f };
                Color m_hoverColor{ 0.22f, 0.22f, 0.26f, 1.0f };
                Color m_checkColor{ 0.2f, 0.7f, 0.4f, 1.0f }; // Зелёный индикатор при включении
                Color m_textColor{ 0.95f, 0.95f, 0.95f, 1.0f };
            
                std::function<void(bool)> m_onChange;
            };
        }
    }
}