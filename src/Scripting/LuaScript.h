#pragma once

#include <string>

#include <sol/sol.hpp>
#include "Component/Component.h"

namespace Lindo::Scripting {

    class LuaScript final : public Lindo::World::Component {
    public:
        explicit LuaScript(std::string scriptPath = {});

        void OnStart() override;
        void OnUpdate() override;
        void OnDestroy() override;

        void setScriptPath(const std::string& path);
        const std::string& getScriptPath() const { return m_scriptPath; }
        bool isLoaded() const { return m_loaded; }
        void setEnabled(bool enabled) { m_enabled = enabled; }
        bool isEnabled() const { return m_enabled; }
        void reload();

    private:
        void call(const char* functionName);
        void callUpdate();

        std::string m_scriptPath;
        sol::environment m_environment;
        bool m_loaded = false;
        bool m_started = false;
        bool m_enabled = true;
    };
}
