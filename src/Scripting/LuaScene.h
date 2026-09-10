#pragma once

#include "world/Scene.h"
#include <sol/sol.hpp>
#include <string>

namespace Lindo::Scripting {

    class LuaScene final : public Lindo::World::Scene {
    public:
        explicit LuaScene(std::string scriptPath);

        void OnCreate() override;
        void OnActivate() override;
        void OnDeactivate() override;
        void Update() override;
        void ProcessInput(Lindo::Input::Input* input) override;
        void OnDestroy() override;

    private:
        void call(const char* name);

        std::string m_scriptPath;
        sol::environment m_environment;
        bool m_loaded = false;
    };
}
