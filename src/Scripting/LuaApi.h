#pragma once

#include <memory>
#include <string>
#include <sol/sol.hpp>

namespace Lindo {
    class SceneManager;
    namespace Input { class Input; }
    namespace Graphics { namespace UI { class UIManager;  } class Renderer; }
    namespace World { class Scene; class GameObject; }
}

namespace Lindo::Scripting {

    class LuaUI {
    public:
        static void SetManager(Lindo::Graphics::UI::UIManager* manager);
        static Lindo::Graphics::UI::UIManager* GetManager();
        static bool LoadXml(const std::string& path, const sol::table& handlers, bool clearExisting = true);
        static void SetChecked(const std::string& id, bool checked);
        static bool IsChecked(const std::string& id);
        static void SetText(const std::string &id, const std::string &text);
        static void SetVisible(const std::string& id, bool visible);
    };

    void BindLuaEngineApi(sol::state& lua);
    void SetLuaEngineContext(Lindo::SceneManager* scenes, Lindo::Input::Input* input,
        Lindo::Graphics::UI::UIManager* ui, Lindo::Graphics::Renderer* renderer);
}
