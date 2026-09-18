#include "LuaApi.h"
#include <memory>
#include "Core/AssetManager.h"
#include "Core/Input.h"
#include "Core/SceneManager.h"
#include "Component/GameObject/GameObject.h"
#include "Component/PlayerController/Player.h"
#include "Component/Camera/Camera.h"
#include "Component/Physhcs/MeshRenderer.h"
#include "Component/Physhcs/RigidBody.h"
#include "Component/Physhcs/Colliders/BoxCollider.h"
#include "Component/Physhcs/Colliders/SphereCollider.h"
#include "Component/Physhcs/Colliders/CapsuleCollider.h"
#include "Component/Physhcs/Colliders/MeshCollider.h"
#include "Component/Graphics/Light.h"
#include "Component/Graphics/Material/Material.h"
#include "Graphics/ui/UIManager.h"
#include "Graphics/ui/UIWidget.h"
#include "Scripting/LuaScript.h"
#include "Scripting/LuaScene.h"
#include "Debug/DebugLogger.h"
#include "Graphics/core/Renderer.h"
#include <fstream>
#include <regex>
#include <sstream>
#include <vector>
#include <unordered_map>
#include <GLFW/glfw3.h>
#include "core/Types/Settings.h"
#include "core/Application.h" 

#include <world/managers/LayerManager.h>

namespace {
    Lindo::SceneManager* g_sceneManager = nullptr;
    Lindo::Input::Input* g_input = nullptr;
    Lindo::Graphics::UI::UIManager* g_ui = nullptr;
    Lindo::Graphics::Renderer* g_renderer = nullptr;
    std::unordered_map<std::string, std::shared_ptr<Lindo::Graphics::UI::UIWidget>> g_namedWidgets;
    std::unordered_map<std::string, std::shared_ptr<Lindo::Graphics::UI::UIWidget>> g_persistentWidgets;

    void restorePersistentWidgets() {
        if (!g_ui || !g_ui->getRootPanel()) return;
        for (const auto& entry : g_persistentWidgets) {
            if (entry.first == "editor.panel") g_ui->getRootPanel()->addChild(entry.second);
        }
    }

    std::string attr(const std::string& text, const std::string& name, const std::string& fallback = {}) {
        const std::regex pattern(name + R"(\s*=\s*["']([^"']*)["'])");
        std::smatch match;
        return std::regex_search(text, match, pattern) ? match[1].str() : fallback;
    }

    float number(const std::string& text, const std::string& name, float fallback = 0.0f) {
        const std::string value = attr(text, name);
        if (value.empty()) return fallback;
        try { return std::stof(value); } catch (...) { return fallback; }
    }

    int integer(const std::string& text, const std::string& name, int fallback = 0) {
        return static_cast<int>(number(text, name, static_cast<float>(fallback)));
    }

    Lindo::Graphics::UI::Color color(const std::string& text, const std::string& name,
        const Lindo::Graphics::UI::Color& fallback) {
        const std::string value = attr(text, name);
        if (value.empty()) return fallback;
        std::stringstream stream(value);
        std::string part;
        std::vector<float> values;
        while (std::getline(stream, part, ',')) {
            try { values.push_back(std::stof(part)); } catch (...) { return fallback; }
        }
        if (values.size() < 3) return fallback;
        return { values[0], values[1], values[2], values.size() > 3 ? values[3] : 1.0f };
    }

    // 1. В applyRect НЕ прибавляем parentX и parentY:
    void applyRect(const std::shared_ptr<Lindo::Graphics::UI::UIWidget>& widget,
                   const std::string& text) {
        if (!widget) return;
                
        widget->setPosition(number(text, "x"), number(text, "y"));
        widget->setSize(number(text, "width", 100.0f), number(text, "height", 40.0f));
        widget->setTextSize(number(text, "fontSize", 24.0f));
                
        int zVal = integer(text, "zOrder", integer(text, "z", 0));
        widget->setZOrder(zVal);

        // Разбор якорей (Unity Style)
        std::string anchorStr = attr(text, "anchor");
        if (!anchorStr.empty()) {
            if (anchorStr == "Center")       widget->setAnchor(Lindo::Graphics::UI::UIAnchor::Center);
            else if (anchorStr == "TopLeft")      widget->setAnchor(Lindo::Graphics::UI::UIAnchor::TopLeft);
            else if (anchorStr == "TopCenter")    widget->setAnchor(Lindo::Graphics::UI::UIAnchor::TopCenter);
            else if (anchorStr == "TopRight")     widget->setAnchor(Lindo::Graphics::UI::UIAnchor::TopRight);
            else if (anchorStr == "CenterLeft")   widget->setAnchor(Lindo::Graphics::UI::UIAnchor::CenterLeft);
            else if (anchorStr == "CenterRight")  widget->setAnchor(Lindo::Graphics::UI::UIAnchor::CenterRight);
            else if (anchorStr == "BottomLeft")   widget->setAnchor(Lindo::Graphics::UI::UIAnchor::BottomLeft);
            else if (anchorStr == "BottomCenter") widget->setAnchor(Lindo::Graphics::UI::UIAnchor::BottomCenter);
            else if (anchorStr == "BottomRight")  widget->setAnchor(Lindo::Graphics::UI::UIAnchor::BottomRight);
        }
    }
}

namespace Lindo::Scripting {

    void SetLuaEngineContext(Lindo::SceneManager* scenes, Lindo::Input::Input* input,
        Lindo::Graphics::UI::UIManager* ui, Lindo::Graphics::Renderer* renderer) {
        g_sceneManager = scenes;
        g_input = input;
        g_ui = ui;
        g_renderer = renderer;
        LuaUI::SetManager(ui);
    }

    void LuaUI::SetManager(Lindo::Graphics::UI::UIManager* manager) { g_ui = manager; }
    Lindo::Graphics::UI::UIManager* LuaUI::GetManager() { return g_ui; }

    bool LuaUI::LoadXml(const std::string& path, const sol::table& handlers, bool clearExisting) {
        if (!g_ui || !g_ui->getRootPanel()) return false;
        const std::string fullPath = AssetManager::get().resolvePath(path, "ui/layouts");
        std::ifstream file(fullPath);
        if (!file) {
            LOG_ERROR("[Lua UI] Cannot open layout: " + fullPath);
            return false;
        }

        if (clearExisting) {
            g_ui->clearDynamicWidgets();
            g_namedWidgets.clear();
            restorePersistentWidgets();
            for (const auto& entry : g_persistentWidgets) g_namedWidgets[entry.first] = entry.second;
        }
        auto root = g_ui->getRootPanel();
        std::vector<std::shared_ptr<Lindo::Graphics::UI::UIPanel>> panels{ root };
        const std::regex tags(R"(<\s*(/?)\s*([A-Za-z]+)([^>]*)>)");
        std::string xml((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        for (std::sregex_iterator it(xml.begin(), xml.end(), tags), end; it != end; ++it) {
            const std::smatch& match = *it;
            const bool closing = !match[1].str().empty();
            const std::string type = match[2].str();
            const std::string attributes = match[3].str();
            if (closing) {
                if ((type == "Panel" || type == "UI") && panels.size() > 1) panels.pop_back();
                continue;
            }
            if (type == "UI") continue;

            std::shared_ptr<Lindo::Graphics::UI::UIWidget> widget;
            float px = (panels.size() > 1) ? panels.back()->getX() : 0.0f;
            float py = (panels.size() > 1) ? panels.back()->getY() : 0.0f;

            if (type == "Panel") {
                auto panel = std::make_shared<Lindo::Graphics::UI::UIPanel>();
                applyRect(panel, attributes);
                panel->SetColor(color(attributes, "color", { 0, 0, 0, 0 }));
                panels.back()->addChild(panel);
                widget = panel;
                if (attributes.empty() || attributes.back() != '/') panels.push_back(panel);
            }
            else if (type == "Label") {
                auto label = std::make_shared<Lindo::Graphics::UI::UILabel>(attr(attributes, "text"));
                applyRect(label, attributes);
                label->setTextColor(color(attributes, "color", { 1, 1, 1, 1 }));
                panels.back()->addChild(label);
                widget = label;
            }
            else if (type == "Button") {
                sol::function callback;
                const std::string handler = attr(attributes, "onClick");
                if (!handler.empty()) {
                    sol::object value = handlers[handler];
                    if (value.is<sol::function>()) callback = value.as<sol::function>();
                }
                auto button = std::make_shared<Lindo::Graphics::UI::UIButton>(attr(attributes, "text"),
                    [callback]() mutable {
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
                panels.back()->addChild(button);
                widget = button;
            }
            else if (type == "Input") {
                auto inputField = std::make_shared<Lindo::Graphics::UI::UITextInput>(attr(attributes, "placeholder"));
                applyRect(inputField, attributes);

                sol::function callback;
                const std::string handler = attr(attributes, "onSubmit");
                if (!handler.empty()) {
                    sol::object value = handlers[handler];
                    if (value.is<sol::function>()) callback = value.as<sol::function>();
                }
                inputField->setOnSubmit([callback](const std::string& val) mutable {
                    if (callback.valid()) callback(val);
                });

                panels.back()->addChild(inputField);
                widget = inputField;
            }
            else if (type == "Slider") {
                float minV = number(attributes, "min", 0.0f);
                float maxV = number(attributes, "max", 1.0f);
                float val = number(attributes, "value", minV);

                auto slider = std::make_shared<Lindo::Graphics::UI::UISlider>(minV, maxV, val);
                applyRect(slider, attributes);

                sol::function callback;
                const std::string handler = attr(attributes, "onChange");
                if (!handler.empty()) {
                    sol::object value = handlers[handler];
                    if (value.is<sol::function>()) callback = value.as<sol::function>();
                }
                slider->setOnChange([callback](float v) mutable {
                    if (callback.valid()) callback(v);
                });

                panels.back()->addChild(slider);
                widget = slider;
            }
            else if (type == "DropDown") {
                std::vector<std::string> options;
                std::string rawOptions = attr(attributes, "options");
                std::stringstream ss(rawOptions);
                std::string opt;
                while (std::getline(ss, opt, ',')) options.push_back(opt);
                        
                auto dropdown = std::make_shared<Lindo::Graphics::UI::UIDropDown>(options);
                applyRect(dropdown, attributes);

                // Начальное значение из XML: "value=2" или "value=High"
                const std::string initial = attr(attributes, "value");
                if (!initial.empty()) {
                    try {
                        dropdown->setSelectedIndex(std::stoi(initial), /*notify*/ false);
                    } catch (...) {
                        for (size_t i = 0; i < options.size(); ++i) {
                            if (options[i] == initial) {
                                dropdown->setSelectedIndex(static_cast<int>(i), false);
                                break;
                            }
                        }
                    }
                }
                        
                sol::function callback;
                const std::string handler = attr(attributes, "onSelect");
                if (!handler.empty()) {
                    sol::object value = handlers[handler];
                    if (value.is<sol::function>()) callback = value.as<sol::function>();
                }
                dropdown->setOnSelect([callback](int idx, const std::string& val) mutable {
                    if (callback.valid()) callback(idx, val);
                });
            
                panels.back()->addChild(dropdown);
                widget = dropdown;
            }
            else if (type == "Toggle") {
                bool checked = (attr(attributes, "checked") == "true" || attr(attributes, "checked") == "1");
                auto toggle = std::make_shared<Lindo::Graphics::UI::UIToggle>(checked);
                applyRect(toggle, attributes);
                
                toggle->setLabel(attr(attributes, "label"));

                sol::function callback;
                const std::string handler = attr(attributes, "onChange");
                if (!handler.empty()) {
                    sol::object value = handlers[handler];
                    if (value.is<sol::function>()) callback = value.as<sol::function>();
                }
                
                toggle->setOnChange([callback](bool state) mutable {
                    if (callback.valid()) {
                        sol::protected_function_result result = callback(state);
                        if (!result.valid()) {
                            sol::error error = result;
                            LOG_ERROR("[Lua UI] Toggle callback failed: " + std::string(error.what()));
                        }
                    }
                });

                panels.back()->addChild(toggle);
                widget = toggle;
            }

            

            if (widget && attributes.find("visible=\"false\"") != std::string::npos) widget->setVisible(false);
            if (widget) {
                const std::string id = attr(attributes, "id");
                if (!id.empty()) {
                    g_namedWidgets[id] = widget;
                    if (!clearExisting) g_persistentWidgets[id] = widget;
                }
            }
        }
        if (g_ui && g_ui->getRootPanel()) {
            auto root = g_ui->getRootPanel();
            root->updateLayout(0.0f, 0.0f, root->getWidth(), root->getHeight());
        }
        return true;

    }
    
    void LuaUI::SetChecked(const std::string& id, bool checked) {
        auto it = g_namedWidgets.find(id);
        if (it == g_namedWidgets.end()) return;
        auto toggle = std::dynamic_pointer_cast<Lindo::Graphics::UI::UIToggle>(it->second);
        if (toggle) toggle->setChecked(checked);
    }

    bool LuaUI::IsChecked(const std::string& id) {
        auto it = g_namedWidgets.find(id);
        if (it == g_namedWidgets.end()) return false;
        auto toggle = std::dynamic_pointer_cast<Lindo::Graphics::UI::UIToggle>(it->second);
        return toggle ? toggle->isChecked() : false;
    }

    void LuaUI::SetText(const std::string& id, const std::string& text) {
        auto it = g_namedWidgets.find(id);
        if (it == g_namedWidgets.end()) return;
        auto label = std::dynamic_pointer_cast<Lindo::Graphics::UI::UILabel>(it->second);
        if (label) label->setText(text);
    }

    void LuaUI::SetVisible(const std::string& id, bool visible) {
        auto it = g_namedWidgets.find(id);
        if (it != g_namedWidgets.end()) it->second->setVisible(visible);
    }

    void BindLuaEngineApi(sol::state& lua) {
        // --- Enums ---
        lua.new_enum<Lindo::ShadowQuality>("ShadowQuality", {
            {"Off", Lindo::ShadowQuality::Off},
            {"Low", Lindo::ShadowQuality::Low},
            {"Medium", Lindo::ShadowQuality::Medium},
            {"High", Lindo::ShadowQuality::High},
            {"Ultra", Lindo::ShadowQuality::Ultra}
        });

        lua.new_enum<Lindo::TextureFiltering>("TextureFiltering", {
            {"Bilinear", Lindo::TextureFiltering::Bilinear},
            {"Trilinear", Lindo::TextureFiltering::Trilinear},
            {"Anisotropic2x", Lindo::TextureFiltering::Anisotropic2x},
            {"Anisotropic8x", Lindo::TextureFiltering::Anisotropic8x},
            {"Anisotropic16x", Lindo::TextureFiltering::Anisotropic16x}
        });

        // --- Settings Binding ---
        lua.new_usertype<Lindo::Settings>("Settings",
            sol::no_constructor,

            "debugMode", &Lindo::Settings::debugMode,
            "showFPS", &Lindo::Settings::showFPS,
            "wireframeMode", &Lindo::Settings::wireframeMode,
            "msaaSamples", &Lindo::Settings::msaaSamples,
            "shadowQuality", &Lindo::Settings::shadowQuality,
            "textureFiltering", &Lindo::Settings::textureFiltering,
            "enableShadows", &Lindo::Settings::enableShadows,
            "fov", &Lindo::Settings::fov,
            "nearPlane", &Lindo::Settings::nearPlane,
            "farPlane", &Lindo::Settings::farPlane,
            "enableBloom", &Lindo::Settings::enableBloom,
            "enableFXAA", &Lindo::Settings::enableFXAA,
            "enableSSAO", &Lindo::Settings::enableSSAO,
            "gamma", &Lindo::Settings::gamma,
            "exposure", &Lindo::Settings::exposure,
            "masterVolume", &Lindo::Settings::masterVolume,
            "musicVolume", &Lindo::Settings::musicVolume,
            "sfxVolume", &Lindo::Settings::sfxVolume,
            "muteAudio", &Lindo::Settings::muteAudio,

            "apply", sol::overload(
                [](Lindo::Settings& s) { s.apply(); },
                [](Lindo::Settings& s, const std::string& path) { s.apply(path); }
            ),
            "resetToDefaults", &Lindo::Settings::resetToDefaults,
            "saveToFile", sol::overload(
                [](Lindo::Settings& s) { s.saveToFile(); },
                [](Lindo::Settings& s, const std::string& path) { s.saveToFile(path); }
            ),
            "loadFromFile", sol::overload(
                [](Lindo::Settings& s) { s.loadFromFile(); },
                [](Lindo::Settings& s, const std::string& path) { s.loadFromFile(path); }
            ),
            "get", &Lindo::Settings::getInstance
        );

        // --- DisplaySettings Binding ---
        lua.new_usertype<Lindo::DisplaySettings>("DisplaySettings",
            sol::no_constructor,

            "windowWidth", &Lindo::DisplaySettings::windowWidth,
            "windowHeight", &Lindo::DisplaySettings::windowHeight,
            "fullscreen", &Lindo::DisplaySettings::fullscreen,
            "borderless", &Lindo::DisplaySettings::borderless,
            "vsync", &Lindo::DisplaySettings::vsync,
            "targetFPS", &Lindo::DisplaySettings::targetFPS,
            "useFixedTimestep", &Lindo::DisplaySettings::useFixedTimestep,
            "fixedTimestep", &Lindo::DisplaySettings::fixedTimestep,

            "getAspectRatio", &Lindo::DisplaySettings::getAspectRatio,
            "apply", sol::overload(
                [](Lindo::DisplaySettings& ds) { ds.apply(); },
                [](Lindo::DisplaySettings& ds, const std::string& path) { ds.apply(path); }
            ),
            "saveToFile", sol::overload(
                [](Lindo::DisplaySettings& ds) { ds.saveToFile(); },
                [](Lindo::DisplaySettings& ds, const std::string& path) { ds.saveToFile(path); }
            ),
            "loadFromFile", sol::overload(
                [](Lindo::DisplaySettings& ds) { ds.loadFromFile(); },
                [](Lindo::DisplaySettings& ds, const std::string& path) { ds.loadFromFile(path); }
            ),
            "get", &Lindo::DisplaySettings::getInstance
        );

        // --- RigidBody Binding ---
        lua.new_usertype<Lindo::Components::Physics::RigidBody>("RigidBody",
            sol::no_constructor,

            // Properties
            "mass", sol::property(&Lindo::Components::Physics::RigidBody::GetMass, &Lindo::Components::Physics::RigidBody::SetMass),
            "invMass", sol::property(&Lindo::Components::Physics::RigidBody::GetInvMass),
            "velocity", sol::property(&Lindo::Components::Physics::RigidBody::GetVelocity, &Lindo::Components::Physics::RigidBody::SetVelocity),
            "angularVelocity", &Lindo::Components::Physics::RigidBody::angularVelocity,
            "acceleration", &Lindo::Components::Physics::RigidBody::acceleration,
            "useGravity", sol::property([](Lindo::Components::Physics::RigidBody& rb) { return rb.useGravity; }, &Lindo::Components::Physics::RigidBody::SetUseGravity),
            "gravityScale", sol::property([](Lindo::Components::Physics::RigidBody& rb) { return rb.gravityScale; }, &Lindo::Components::Physics::RigidBody::SetGravityScale),
            "restitution", &Lindo::Components::Physics::RigidBody::restitution,
            "friction", &Lindo::Components::Physics::RigidBody::friction,
            "linearDamping", &Lindo::Components::Physics::RigidBody::linearDamping,
            "angularDamping", &Lindo::Components::Physics::RigidBody::angularDamping,
            "isKinematic", sol::property(&Lindo::Components::Physics::RigidBody::IsKinematic, &Lindo::Components::Physics::RigidBody::SetKinematic),
            "isSleeping", &Lindo::Components::Physics::RigidBody::isSleeping,
            "isGrounded", &Lindo::Components::Physics::RigidBody::isGrounded,

            // Methods
            "applyForce", &Lindo::Components::Physics::RigidBody::applyForce,
            "applyForceAtPoint", &Lindo::Components::Physics::RigidBody::applyForceAtPoint,
            "applyImpulse", &Lindo::Components::Physics::RigidBody::applyImpulse,
            "applyImpulseAtPoint", &Lindo::Components::Physics::RigidBody::applyImpulseAtPoint,
            "applyTorque", &Lindo::Components::Physics::RigidBody::applyTorque,
            "clearForces", &Lindo::Components::Physics::RigidBody::clearForces,
            "wake", &Lindo::Components::Physics::RigidBody::Wake
        );

        lua.new_usertype<Lindo::World::Scene>("Scene",
            "name", sol::property(&Lindo::World::Scene::GetName, &Lindo::World::Scene::SetName),
            "active", sol::property(&Lindo::World::Scene::IsActive, &Lindo::World::Scene::SetActive),
            "create", &Lindo::World::Scene::CreateGameObject,
            "find", &Lindo::World::Scene::FindGameObject,
            "destroy", static_cast<void (Lindo::World::Scene::*)(const std::string&)>(
                &Lindo::World::Scene::DestroyGameObject));

        lua.new_usertype<Lindo::Scripting::LuaScene>("LuaScene",
            sol::base_classes, sol::bases<Lindo::World::Scene>());

        auto layerTable = lua.create_named_table("LayerMask");
        layerTable["getMask"] = [](sol::variadic_args args) {
            std::vector<std::string> names;
            for (auto arg : args) {
                if (arg.is<std::string>()) names.push_back(arg.as<std::string>());
            }
            return Lindo::World::LayerManager::get().getMask(names);
        };
        layerTable["nameToLayer"] = [](const std::string& name) {
            return Lindo::World::LayerManager::get().getLayerByName(name);
        };
        layerTable["layerToName"] = [](uint8_t layer) {
            return Lindo::World::LayerManager::get().getLayerName(layer);
        };

        lua.new_usertype<Lindo::World::GameObject>("GameObject",
            "name", sol::property(&Lindo::World::GameObject::getName, &Lindo::World::GameObject::setName),
            "tag", sol::property(&Lindo::World::GameObject::getTag, &Lindo::World::GameObject::setTag),
            "layer", sol::property(&Lindo::World::GameObject::getLayer, &Lindo::World::GameObject::setLayer),
            "layerName", sol::property(&Lindo::World::GameObject::getLayerName, &Lindo::World::GameObject::setLayerByName),
            "compareTag", &Lindo::World::GameObject::compareTag,
            "active", sol::property(
                [](Lindo::World::GameObject& object) { return object.isActive; },
                [](Lindo::World::GameObject& object, bool value) { object.isActive = value; }),
            "transform", sol::property(
                [](Lindo::World::GameObject& object) -> Lindo::Math::Transform& { return object.transform; }),
            "getWorldPosition", [](Lindo::World::GameObject& object) { return object.getWorldPosition(); },
            "setWorldPosition", [](Lindo::World::GameObject& object, const glm::vec3& position) {
                object.setWorldPosition(position);
            },
            "addComponent", [](Lindo::World::GameObject& object, const std::string& type) {
                if (type == "Player") object.getOrAddComponent<Lindo::Components::Controller::Player>();
                else if (type == "Camera") object.getOrAddComponent<Lindo::Components::Rendering::Camera>();
                else if (type == "MeshRenderer") object.getOrAddComponent<Lindo::Components::Physics::MeshRenderer>();
                else if (type == "Material") object.getOrAddComponent<Lindo::Graphics::Material>(0, 0, 32.0f, false);
                else if (type == "BoxCollider") object.getOrAddComponent<Lindo::Components::Physics::BoxCollider>();
                else if (type == "SphereCollider") object.getOrAddComponent<Lindo::Components::Physics::SphereCollider>();
                else if (type == "CapsuleCollider") object.getOrAddComponent<Lindo::Components::Physics::CapsuleCollider>();
                else if (type == "MeshCollider") object.getOrAddComponent<Lindo::Components::Physics::MeshCollider>();
                else if (type == "DirectionalLight") object.getOrAddComponent<Lindo::Components::Light::DirectionalLight>();
                else if (type == "RigidBody") object.getOrAddComponent<Lindo::Components::Physics::RigidBody>();
            },
            "getRigidBody", [](Lindo::World::GameObject& object) {
                return object.getComponent<Lindo::Components::Physics::RigidBody>();
            },
            "setModel", [](Lindo::World::GameObject& object, const std::string& path) {
                auto* renderer = object.getOrAddComponent<Lindo::Components::Physics::MeshRenderer>();
                renderer->setModel(Lindo::AssetManager::get().loadModel(path));
            },
            "setMaterialColor", [](Lindo::World::GameObject& object, const glm::vec3& value) {
                auto* material = object.getOrAddComponent<Lindo::Graphics::Material>(0, 0, 32.0f, false);
                material->setColor(value);
                if (auto* renderer = object.getComponent<Lindo::Components::Physics::MeshRenderer>()) {
                    renderer->material = material;
                }
            },
            // Assigns a diffuse texture (and optionally a specular map) to the object's material.
            // Any color previously set with setMaterialColor keeps acting as a tint on top of the
            // texture, so texture and color can be combined instead of one overriding the other.
            "setMaterialTexture", [](Lindo::World::GameObject& object, const std::string& diffusePath,
                sol::optional<std::string> specularPath) {
                auto* material = object.getOrAddComponent<Lindo::Graphics::Material>(0, 0, 32.0f, false);
                const unsigned int diffuseTex = Lindo::AssetManager::get().loadTexture(diffusePath);
                material->setDiffuseTexture(diffuseTex);
                if (specularPath) {
                    const unsigned int specularTex = Lindo::AssetManager::get().loadTexture(*specularPath);
                    material->setSpecularTexture(specularTex);
                }
                if (auto* renderer = object.getComponent<Lindo::Components::Physics::MeshRenderer>()) {
                    renderer->material = material;
                }
            },
            // Removes any texture from the object's material, falling back to a flat material.color.
            "clearMaterialTexture", [](Lindo::World::GameObject& object) {
                if (auto* material = object.getComponent<Lindo::Graphics::Material>()) {
                    material->clearTexture();
                }
            },
            "setMaterialShininess", [](Lindo::World::GameObject& object, float shininess) {
                auto* material = object.getOrAddComponent<Lindo::Graphics::Material>(0, 0, 32.0f, false);
                material->shininess = shininess;
                if (auto* renderer = object.getComponent<Lindo::Components::Physics::MeshRenderer>()) {
                    renderer->material = material;
                }
            },
            "setMaterialTwoSided", [](Lindo::World::GameObject& object, bool twoSided) {
                auto* material = object.getOrAddComponent<Lindo::Graphics::Material>(0, 0, 32.0f, false);
                material->twoSided = twoSided;
                if (auto* renderer = object.getComponent<Lindo::Components::Physics::MeshRenderer>()) {
                    renderer->material = material;
                }
            },
 
            "setMaterialTiling", [](Lindo::World::GameObject& object, float tx, float ty) {
                auto* material = object.getOrAddComponent<Lindo::Graphics::Material>(0, 0, 32.0f, false);
                material->setDiffuseTiling(tx, ty);
                if (auto* renderer = object.getComponent<Lindo::Components::Physics::MeshRenderer>())
                    renderer->material = material;
            },
            "setMaterialOffset", [](Lindo::World::GameObject& object, float ox, float oy) {
                auto* material = object.getOrAddComponent<Lindo::Graphics::Material>(0, 0, 32.0f, false);
                material->setDiffuseOffset(ox, oy);
                if (auto* renderer = object.getComponent<Lindo::Components::Physics::MeshRenderer>())
                    renderer->material = material;
            },
            "setMaterialUV", [](Lindo::World::GameObject& object,
                                float tx, float ty, float ox, float oy) {
                auto* material = object.getOrAddComponent<Lindo::Graphics::Material>(0, 0, 32.0f, false);
                material->setDiffuseUV(tx, ty, ox, oy);
                if (auto* renderer = object.getComponent<Lindo::Components::Physics::MeshRenderer>())
                    renderer->material = material;
            },
            "setMaterialSpecularUV", [](Lindo::World::GameObject& object,
                                        float tx, float ty, float ox, float oy) {
                auto* material = object.getOrAddComponent<Lindo::Graphics::Material>(0, 0, 32.0f, false);
                material->setSpecularUV(tx, ty, ox, oy);
                if (auto* renderer = object.getComponent<Lindo::Components::Physics::MeshRenderer>())
                    renderer->material = material;
            },
            "fitCollider", [](Lindo::World::GameObject& object) {
                auto* renderer = object.getComponent<Lindo::Components::Physics::MeshRenderer>();
                if (!renderer || !renderer->model) return;
                const auto aabb = renderer->model->getAABB();
                if (auto* collider = object.getComponent<Lindo::Components::Physics::BoxCollider>()) collider->FitToAABB(aabb);
                if (auto* collider = object.getComponent<Lindo::Components::Physics::SphereCollider>()) collider->FitToAABB(aabb);
                if (auto* collider = object.getComponent<Lindo::Components::Physics::CapsuleCollider>()) collider->FitToAABB(aabb);
                if (auto* collider = object.getComponent<Lindo::Components::Physics::MeshCollider>()) collider->UpdateFromMeshRenderer();
            });

        auto input = lua.create_named_table("Input");
        input["action"] = [](const std::string& name) { return g_input && g_input->getAction(name); };
        input["actionDown"] = [](const std::string& name) { return g_input && g_input->getActionDown(name); };
        input["actionUp"] = [](const std::string& name) { return g_input && g_input->getActionUp(name); };
        input["getKey"] = [](const std::string& name) { return g_input && g_input->getKey(name); };
        input["getKeyDown"] = [](const std::string& name) { return g_input && g_input->getKeyDown(name); };
        input["getKeyUp"] = [](const std::string& name) { return g_input && g_input->getKeyUp(name); };
        input["consumeEscape"] = []() { return g_input && g_input->consumeEscape(); };
        input["axis"] = [](const std::string& positive, const std::string& negative) { return g_input ? g_input->getAxis(positive, negative) : 0.0f; };
        input["setUIActive"] = [](bool active) { if (g_input) g_input->setUIActive(active); };
        input["isUIActive"] = []() { return g_input && g_input->isUIActive(); };

        lua.new_enum<Lindo::Graphics::UI::UIAnchor>("UIAnchor", {
            {"None", Lindo::Graphics::UI::UIAnchor::None},
            {"TopLeft", Lindo::Graphics::UI::UIAnchor::TopLeft},
            {"TopCenter", Lindo::Graphics::UI::UIAnchor::TopCenter},
            {"TopRight", Lindo::Graphics::UI::UIAnchor::TopRight},
            {"CenterLeft", Lindo::Graphics::UI::UIAnchor::CenterLeft},
            {"Center", Lindo::Graphics::UI::UIAnchor::Center},
            {"CenterRight", Lindo::Graphics::UI::UIAnchor::CenterRight},
            {"BottomLeft", Lindo::Graphics::UI::UIAnchor::BottomLeft},
            {"BottomCenter", Lindo::Graphics::UI::UIAnchor::BottomCenter},
            {"BottomRight", Lindo::Graphics::UI::UIAnchor::BottomRight}
        });

        // Добавь setAnchor в usertype UIWidget:
        lua.new_usertype<Lindo::Graphics::UI::UIWidget>("UIWidget",
            "tag", sol::property(&Lindo::Graphics::UI::UIWidget::getTag, &Lindo::Graphics::UI::UIWidget::setTag),
            "layer", sol::property(&Lindo::Graphics::UI::UIWidget::getLayer, &Lindo::Graphics::UI::UIWidget::setLayer),
            "layerName", sol::property(&Lindo::Graphics::UI::UIWidget::getLayerName, &Lindo::Graphics::UI::UIWidget::setLayerByName),
            "compareTag", &Lindo::Graphics::UI::UIWidget::compareTag,
            "zOrder", sol::property(&Lindo::Graphics::UI::UIWidget::getZOrder, &Lindo::Graphics::UI::UIWidget::setZOrder),
            "visible", sol::property(&Lindo::Graphics::UI::UIWidget::isVisible, &Lindo::Graphics::UI::UIWidget::setVisible),
            "setAnchor", &Lindo::Graphics::UI::UIWidget::setAnchor
        );
        lua.new_usertype<Lindo::Graphics::UI::UIPanel>("UIPanel", sol::base_classes, sol::bases<Lindo::Graphics::UI::UIWidget>(),
            "addChild", &Lindo::Graphics::UI::UIPanel::addChild);
        lua.new_usertype<Lindo::Graphics::UI::UILabel>("UILabel", sol::base_classes, sol::bases<Lindo::Graphics::UI::UIWidget>(),
            "setText", &Lindo::Graphics::UI::UILabel::setText);

        lua.new_usertype<Lindo::Graphics::UI::UITextInput>("UITextInput", 
            sol::base_classes, sol::bases<Lindo::Graphics::UI::UIWidget>(),
            "setText", &Lindo::Graphics::UI::UITextInput::setText,
            "getText", &Lindo::Graphics::UI::UITextInput::getText);

        lua.new_usertype<Lindo::Graphics::UI::UISlider>("UISlider",
            sol::base_classes, sol::bases<Lindo::Graphics::UI::UIWidget>(),
            "setValue", [](Lindo::Graphics::UI::UISlider& s, float v) { s.setValue(v); },
            "getValue", &Lindo::Graphics::UI::UISlider::getValue);

        lua.new_usertype<Lindo::Graphics::UI::UIDropDown>("UIDropDown",
            sol::base_classes, sol::bases<Lindo::Graphics::UI::UIWidget>(),
            "getSelectedIndex",  &Lindo::Graphics::UI::UIDropDown::getSelectedIndex,
            "getSelectedOption", &Lindo::Graphics::UI::UIDropDown::getSelectedOption,
            "setSelectedIndex",  [](Lindo::Graphics::UI::UIDropDown& d, int i) {
                d.setSelectedIndex(i);
            });

        lua.new_usertype<Lindo::Graphics::UI::UIToggle>("UIToggle",
            sol::base_classes, sol::bases<Lindo::Graphics::UI::UIWidget>(),
            "setChecked", &Lindo::Graphics::UI::UIToggle::setChecked,
            "isChecked", &Lindo::Graphics::UI::UIToggle::isChecked,
            "setLabel", &Lindo::Graphics::UI::UIToggle::setLabel,
            "getLabel", &Lindo::Graphics::UI::UIToggle::getLabel
        );

        auto ui = lua.create_named_table("UI");

        ui["setAnchor"] = [](const std::string& id, Lindo::Graphics::UI::UIAnchor anchor) {
            auto it = g_namedWidgets.find(id);
            if (it == g_namedWidgets.end()) return;
        
            it->second->setAnchor(anchor);
        
            if (g_ui && g_ui->getRootPanel()) {
                auto root = g_ui->getRootPanel();
                root->updateLayout(0.0f, 0.0f, root->getWidth(), root->getHeight());
            }
        };
        ui["createPanel"] = []() { return std::make_shared<Lindo::Graphics::UI::UIPanel>(); };
        ui["createLabel"] = [](const std::string& text) { return std::make_shared<Lindo::Graphics::UI::UILabel>(text); };
        ui["createButton"] = [](const std::string& text, sol::function callback) {
            return std::make_shared<Lindo::Graphics::UI::UIButton>(text, [callback]() mutable { callback(); });
        };
        ui["root"] = []() { return g_ui ? g_ui->getRootPanel() : std::shared_ptr<Lindo::Graphics::UI::UIPanel>(); };
        ui["clear"] = []() {
            if (!g_ui) return;
            g_ui->clearDynamicWidgets();
            g_namedWidgets.clear();
            restorePersistentWidgets();
        };
        ui["loadXml"] = [](const std::string& path, const sol::table& handlers) {
            return LuaUI::LoadXml(path, handlers, true);
        };
        ui["addXml"] = [](const std::string& path, const sol::table& handlers) {
            return LuaUI::LoadXml(path, handlers, false);
        };
        ui["setText"] = &LuaUI::SetText;
        ui["setVisible"] = &LuaUI::SetVisible;

        ui["setChecked"] = [](const std::string& id, bool checked) {
            LuaUI::SetChecked(id, checked);
        };
        
        ui["isChecked"] = [](const std::string& id) -> bool {
            return LuaUI::IsChecked(id);
        };

        ui["getValue"] = [&lua](const std::string& id) -> sol::object {
            auto it = g_namedWidgets.find(id);
            if (it == g_namedWidgets.end()) return sol::nil;

            if (auto slider = std::dynamic_pointer_cast<Lindo::Graphics::UI::UISlider>(it->second)) {
                return sol::make_object(lua.lua_state(), slider->getValue());
            }
            if (auto input = std::dynamic_pointer_cast<Lindo::Graphics::UI::UITextInput>(it->second)) {
                return sol::make_object(lua.lua_state(), input->getText());
            }
            if (auto dropdown = std::dynamic_pointer_cast<Lindo::Graphics::UI::UIDropDown>(it->second)) {
                return sol::make_object(lua.lua_state(), dropdown->getSelectedOption());
            }
            if (auto toggle = std::dynamic_pointer_cast<Lindo::Graphics::UI::UIToggle>(it->second)) {
                return sol::make_object(lua.lua_state(), toggle->isChecked());
            }
            return sol::nil;
        };
        

        // --- Slider ---
        ui["setSliderValue"] = [](const std::string& id, float value) {
            auto it = g_namedWidgets.find(id);
            if (it == g_namedWidgets.end()) return;
            if (auto s = std::dynamic_pointer_cast<Lindo::Graphics::UI::UISlider>(it->second))
                s->setValue(value, /*notify*/ false);   // тихо, без callback
        };
        ui["getSliderValue"] = [](const std::string& id) -> float {
            auto it = g_namedWidgets.find(id);
            if (it == g_namedWidgets.end()) return 0.0f;
            if (auto s = std::dynamic_pointer_cast<Lindo::Graphics::UI::UISlider>(it->second))
                return s->getValue();
            return 0.0f;
        };

        // --- DropDown ---
        ui["setDropdownIndex"] = [](const std::string& id, int index) {
            auto it = g_namedWidgets.find(id);
            if (it == g_namedWidgets.end()) return;
            if (auto d = std::dynamic_pointer_cast<Lindo::Graphics::UI::UIDropDown>(it->second))
                d->setSelectedIndex(index, /*notify*/ false);
        };
        ui["getDropdownIndex"] = [](const std::string& id) -> int {
            auto it = g_namedWidgets.find(id);
            if (it == g_namedWidgets.end()) return -1;
            if (auto d = std::dynamic_pointer_cast<Lindo::Graphics::UI::UIDropDown>(it->second))
                return d->getSelectedIndex();
            return -1;
        };

        // --- TextInput ---
        ui["setInputText"] = [](const std::string& id, const std::string& text) {
            auto it = g_namedWidgets.find(id);
            if (it == g_namedWidgets.end()) return;
            if (auto inp = std::dynamic_pointer_cast<Lindo::Graphics::UI::UITextInput>(it->second))
                inp->setText(text);
        };
        ui["focusInput"] = [](const std::string& id, bool focus) {
            auto it = g_namedWidgets.find(id);
            if (it == g_namedWidgets.end()) return;
            if (auto inp = std::dynamic_pointer_cast<Lindo::Graphics::UI::UITextInput>(it->second))
                inp->setFocused(focus);
        };
        // ============================================================
        // UI CENTERING
        // ============================================================
            
        ui["center"] = [](const std::string& id) {
        
            auto it = g_namedWidgets.find(id);
        
            if (it == g_namedWidgets.end()) {
                LOG_WARN("[Lua UI] center: unknown widget id '" + id + "'");
                return;
            }
        
            GLFWwindow* win = glfwGetCurrentContext();
        
            if (!win)
                return;
        
            int windowWidth = 0;
            int windowHeight = 0;
        
            glfwGetWindowSize(win, &windowWidth, &windowHeight);
        
            auto& widget = it->second;
        
            const float widgetWidth  = widget->getWidth();
            const float widgetHeight = widget->getHeight();
        
            const float x =
                (static_cast<float>(windowWidth) - widgetWidth) * 0.5f;
        
            const float y =
                (static_cast<float>(windowHeight) - widgetHeight) * 0.5f;
        
            widget->setPosition(x, y);
        };
        
        
        // ============================================================
        // CENTER X
        // ============================================================
        
        ui["centerX"] = [](const std::string& id) {
        
            auto it = g_namedWidgets.find(id);
        
            if (it == g_namedWidgets.end())
                return;
        
            GLFWwindow* win = glfwGetCurrentContext();
        
            if (!win)
                return;
        
            int windowWidth = 0;
            int windowHeight = 0;
        
            glfwGetWindowSize(win, &windowWidth, &windowHeight);
        
            auto& widget = it->second;
        
            const float x =
                (static_cast<float>(windowWidth) - widget->getWidth()) * 0.5f;
        
            widget->setPosition(x, widget->getY());
        };

        auto renderer = lua.create_named_table("Renderer");
        renderer["setSkybox"] = [](const std::string& path, sol::optional<int> res) {
            if (g_renderer) {
                return g_renderer->setSkybox(path, res.value_or(1024));
            }
            return false;
        };

        auto scenes = lua.create_named_table("Scenes");
        scenes["current"] = []() { return g_sceneManager ? g_sceneManager->GetCurrentScene() : nullptr; };
        scenes["load"] = [](const std::string& name) { if (g_sceneManager) g_sceneManager->RequestLoadScene(name); };
        scenes["register"] = [](const std::string& name, const std::string& script) {
            if (g_sceneManager) g_sceneManager->RegisterSceneFactory(name, [script]() {
                return std::make_unique<Lindo::Scripting::LuaScene>(script);
            });
        };
        scenes["setSkybox"] = [](const std::string& path, sol::optional<int> res) {
            if (g_renderer) {
                return g_renderer->setSkybox(path, res.value_or(1024));
            }
            return false;
        };

        auto application = lua.create_named_table("Application");
        application["quit"] = []() {
            if (GLFWwindow* window = glfwGetCurrentContext()) {
                glfwSetWindowShouldClose(window, GLFW_TRUE);
            }
        };

        application["windowWidth"] = []() {
            int w = 0, h = 0;
            if (GLFWwindow* win = glfwGetCurrentContext()) glfwGetWindowSize(win, &w, &h);
            return w;
        };
        application["windowHeight"] = []() {
            int w = 0, h = 0;
            if (GLFWwindow* win = glfwGetCurrentContext()) glfwGetWindowSize(win, &w, &h);
            return h;
        };

        // --- Информация о приложении (единый источник — AppInfo) ---
        application["name"]       = Lindo::AppInfo::Name;
        application["version"]    = Lindo::AppInfo::GetVersionString();      // "26.2.2-dev"
        application["title"]      = Lindo::AppInfo::GetFormattedTitle();     // "Lindo v26.2.2-dev [Debug]"
        application["major"]      = Lindo::AppInfo::VersionMajor;            // 26
        application["minor"]      = Lindo::AppInfo::VersionMinor;            // 2
        application["patch"]      = Lindo::AppInfo::VersionPatch;            // 2
        application["stage"]      = Lindo::AppInfo::Stage;                   // "dev"
    }
}