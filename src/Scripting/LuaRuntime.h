#pragma once

#include <memory>
#include <string>

#include <sol/sol.hpp>

namespace Lindo::Scripting {

    class LuaRuntime {
    public:
        static LuaRuntime& Get();

        bool initialize();
        bool executeFile(const std::string& path, sol::environment& environment);
        void reportError(const std::string& source, const sol::error& error) const;
        sol::state& state();
        bool isInitialized() const { return m_initialized; }

    private:
        LuaRuntime() = default;
        void bindApi();

        std::unique_ptr<sol::state> m_state;
        bool m_initialized = false;
    };
}
