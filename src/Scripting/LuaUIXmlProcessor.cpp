#include "LuaUIXmlProcessor.h"

#include "UIStyleSheet.h"
#include "Graphics/ui/UIWidget.h"
#include "Debug/DebugLogger.h"

#include <sstream>

namespace Lindo::Scripting {
    namespace {
        using Attributes = std::unordered_map<std::string, std::string>;
        using Widget = Graphics::UI::UIWidget;
        using Panel = Graphics::UI::UIPanel;
        using Color = Graphics::UI::Color;

        std::string value(const Attributes& attributes, const std::string& key, const std::string& fallback = {}) {
            const auto it = attributes.find(key);
            return it == attributes.end() ? fallback : it->second;
        }

        float number(const Attributes& attributes, const std::string& key, float fallback = 0.0f) {
            try { return std::stof(value(attributes, key)); } catch (...) { return fallback; }
        }

        Color color(const Attributes& attributes, const std::string& key, const Color& fallback) {
            std::stringstream stream(value(attributes, key));
            std::string part;
            float values[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
            int count = 0;
            while (count < 4 && std::getline(stream, part, ',')) {
                try { values[count++] = std::stof(part); } catch (...) { return fallback; }
            }
            if (count < 3) return fallback;
            return { values[0], values[1], values[2], values[3] };
        }

        void applyRect(const std::shared_ptr<Widget>& widget, const Attributes& attributes) {
            widget->setPosition(number(attributes, "x"), number(attributes, "y"));
            widget->setSize(number(attributes, "width", 100.0f), number(attributes, "height", 40.0f));
            widget->setTextSize(number(attributes, "fontSize", 24.0f));
            widget->setZOrder(static_cast<int>(number(attributes, "zOrder", number(attributes, "z"))));

            const std::string anchor = value(attributes, "anchor");
            using Graphics::UI::UIAnchor;
            if (anchor == "Center") widget->setAnchor(UIAnchor::Center);
            else if (anchor == "TopLeft") widget->setAnchor(UIAnchor::TopLeft);
            else if (anchor == "TopCenter") widget->setAnchor(UIAnchor::TopCenter);
            else if (anchor == "TopRight") widget->setAnchor(UIAnchor::TopRight);
            else if (anchor == "CenterLeft") widget->setAnchor(UIAnchor::CenterLeft);
            else if (anchor == "CenterRight") widget->setAnchor(UIAnchor::CenterRight);
            else if (anchor == "BottomLeft") widget->setAnchor(UIAnchor::BottomLeft);
            else if (anchor == "BottomCenter") widget->setAnchor(UIAnchor::BottomCenter);
            else if (anchor == "BottomRight") widget->setAnchor(UIAnchor::BottomRight);
        }

        sol::function handler(const sol::table& handlers, const Attributes& attributes, const std::string& key) {
            const std::string name = value(attributes, key);
            if (name.empty()) return {};
            sol::object callback = handlers[name];
            return callback.is<sol::function>() ? callback.as<sol::function>() : sol::function();
        }

        void processChildren(const UIXmlElement& element, const std::shared_ptr<Panel>& parent,
            const sol::table& handlers, const UIStyleSheet& styles,
            std::unordered_map<std::string, std::shared_ptr<Widget>>& namedWidgets) {
            for (const UIXmlElement& child : element.children) {
                const Attributes attributes = styles.resolve(child);
                std::shared_ptr<Widget> widget;

                if (child.name == "Panel") {
                    auto panel = std::make_shared<Panel>();
                    applyRect(panel, attributes);
                    panel->SetColor(color(attributes, "color", { 0, 0, 0, 0 }));
                    parent->addChild(panel);
                    widget = panel;
                    processChildren(child, panel, handlers, styles, namedWidgets);
                } else if (child.name == "Label") {
                    auto label = std::make_shared<Graphics::UI::UILabel>(value(attributes, "text"));
                    applyRect(label, attributes);
                    label->setTextColor(color(attributes, "color", { 1, 1, 1, 1 }));
                    parent->addChild(label);
                    widget = label;
                } else if (child.name == "Button") {
                    sol::function callback = handler(handlers, attributes, "onClick");
                    auto button = std::make_shared<Graphics::UI::UIButton>(value(attributes, "text"), [callback]() mutable {
                        if (callback.valid()) {
                            sol::protected_function_result result = callback();
                            if (!result.valid()) {
                                sol::error error = result;
                                LOG_ERROR("[Lua UI] Button callback failed: " + std::string(error.what()));
                            }
                        }
                    });
                    applyRect(button, attributes);
                    button->setTextColor(color(attributes, "textColor", { 1, 1, 1, 1 }));
                    button->setNormalColor(color(attributes, "color", { 0.2f, 0.2f, 0.2f, 1 }));
                    parent->addChild(button);
                    widget = button;
                } else if (child.name == "Input") {
                    auto input = std::make_shared<Graphics::UI::UITextInput>(value(attributes, "placeholder"));
                    applyRect(input, attributes);
                    input->setText(value(attributes, "value"));
                    sol::function callback = handler(handlers, attributes, "onSubmit");
                    input->setOnSubmit([callback](const std::string& text) mutable {
                        if (callback.valid()) {
                            sol::protected_function_result result = callback(text);
                            if (!result.valid()) { sol::error error = result; LOG_ERROR("[Lua UI] Input callback failed: " + std::string(error.what())); }
                        }
                    });
                    parent->addChild(input);
                    widget = input;
                } else if (child.name == "Slider") {
                    const float minimum = number(attributes, "min");
                    const float maximum = number(attributes, "max", 1.0f);
                    auto slider = std::make_shared<Graphics::UI::UISlider>(minimum, maximum, number(attributes, "value", minimum));
                    applyRect(slider, attributes);
                    sol::function callback = handler(handlers, attributes, "onChange");
                    slider->setOnChange([callback](float current) mutable {
                        if (callback.valid()) {
                            sol::protected_function_result result = callback(current);
                            if (!result.valid()) { sol::error error = result; LOG_ERROR("[Lua UI] Slider callback failed: " + std::string(error.what())); }
                        }
                    });
                    parent->addChild(slider);
                    widget = slider;
                } else if (child.name == "DropDown") {
                    std::vector<std::string> options;
                    std::stringstream stream(value(attributes, "options"));
                    std::string option;
                    while (std::getline(stream, option, ',')) options.push_back(option);
                    auto dropdown = std::make_shared<Graphics::UI::UIDropDown>(options);
                    applyRect(dropdown, attributes);
                    const std::string initial = value(attributes, "value");
                    if (!initial.empty()) {
                        try { dropdown->setSelectedIndex(std::stoi(initial), false); }
                        catch (...) {
                            for (size_t i = 0; i < options.size(); ++i) {
                                if (options[i] == initial) { dropdown->setSelectedIndex(static_cast<int>(i), false); break; }
                            }
                        }
                    }
                    sol::function callback = handler(handlers, attributes, "onSelect");
                    dropdown->setOnSelect([callback](int index, const std::string& selected) mutable {
                        if (callback.valid()) {
                            sol::protected_function_result result = callback(index, selected);
                            if (!result.valid()) { sol::error error = result; LOG_ERROR("[Lua UI] DropDown callback failed: " + std::string(error.what())); }
                        }
                    });
                    parent->addChild(dropdown);
                    widget = dropdown;
                } else if (child.name == "Toggle") {
                    const std::string checked = value(attributes, "checked");
                    auto toggle = std::make_shared<Graphics::UI::UIToggle>(checked == "true" || checked == "1");
                    applyRect(toggle, attributes);
                    toggle->setLabel(value(attributes, "label"));
                    sol::function callback = handler(handlers, attributes, "onChange");
                    toggle->setOnChange([callback](bool state) mutable {
                        if (callback.valid()) {
                            sol::protected_function_result result = callback(state);
                            if (!result.valid()) { sol::error error = result; LOG_ERROR("[Lua UI] Toggle callback failed: " + std::string(error.what())); }
                        }
                    });
                    parent->addChild(toggle);
                    widget = toggle;
                }

                if (!widget) continue;
                if (value(attributes, "visible", "true") == "false") widget->setVisible(false);
                const std::string id = value(attributes, "id");
                if (!id.empty()) namedWidgets[id] = widget;
            }
        }
    }

    void LuaUIXmlProcessor::build(const UIXmlElement& document,
        const std::shared_ptr<Graphics::UI::UIPanel>& root,
        const sol::table& handlers,
        const UIStyleSheet& styles,
        std::unordered_map<std::string, std::shared_ptr<Graphics::UI::UIWidget>>& namedWidgets) {
        if (root) processChildren(document, root, handlers, styles, namedWidgets);
    }

}