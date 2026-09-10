#include "LuaRuntime.h"

#include "Core/Time/Time.h"
#include "Debug/DebugLogger.h"
#include "Scripting/LuaApi.h"
#include "Component/GameObject/transform.h"
#include <glm/glm.hpp>

namespace Lindo::Scripting {

    LuaRuntime& LuaRuntime::Get() {
        static LuaRuntime instance;
        return instance;
    }

    bool LuaRuntime::initialize() {
        if (m_initialized) return true;

        try {
            m_state = std::make_unique<sol::state>();
            m_state->open_libraries(
                sol::lib::base, 
                sol::lib::math, 
                sol::lib::table,
                sol::lib::string, 
                sol::lib::coroutine,
                sol::lib::debug
            );

            // Устанавливаем debug.traceback как дефолтный обработчик ошибок
            sol::object mainTraceback = (*m_state)["debug"]["traceback"];
            if (mainTraceback.is<sol::protected_function>()) {
                sol::protected_function::set_default_handler(mainTraceback.as<sol::protected_function>());
            }

            bindApi();
            m_initialized = true;
            LOG_INFO("[Lua] Runtime initialized successfully.");
            return true;
        }
        catch (const std::exception& error) {
            LOG_ERROR("[Lua] Runtime initialization failed: " + std::string(error.what()));
            m_state.reset();
            return false;
        }
    }

    void LuaRuntime::bindApi() {
        auto& lua = *m_state;

        lua.new_usertype<glm::vec3>("Vector3",
            sol::constructors<glm::vec3(), glm::vec3(float, float, float)>(),
            "x", &glm::vec3::x,
            "y", &glm::vec3::y,
            "z", &glm::vec3::z);
        lua.set_function("Vector3", sol::overload(
            []() { return glm::vec3(0.0f); },
            [](float x, float y, float z) { return glm::vec3(x, y, z); }));

        lua.new_usertype<Lindo::Math::Transform>("Transform",
            "position", sol::property(
                [](Lindo::Math::Transform& transform) { return transform.position; },
                [](Lindo::Math::Transform& transform, const glm::vec3& value) { transform.position = value; }),
            "rotation", sol::property(
                [](Lindo::Math::Transform& transform) { return transform.rotation; },
                [](Lindo::Math::Transform& transform, const glm::vec3& value) { transform.rotation = value; }),
            "scale", sol::property(
                [](Lindo::Math::Transform& transform) { return transform.scale; },
                [](Lindo::Math::Transform& transform, const glm::vec3& value) { transform.scale = value; }));

        BindLuaEngineApi(lua);

        auto time = lua.create_named_table("Time");
        time["deltaTime"] = []() { return Lindo::Time::GetDeltaTime(); };
        time["unscaledDeltaTime"] = []() { return Lindo::Time::GetUnscaledDeltaTime(); };
        time["time"] = []() { return Lindo::Time::GetTotalTime(); };
        time["timeScale"] = []() { return Lindo::Time::GetTimeScale(); };
        time["setTimeScale"] = [](float value) { Lindo::Time::SetTimeScale(value); };

        auto debug = lua.create_named_table("Debug");
        debug["log"] = [](const std::string& msg) { LOG_INFO(msg); };
        debug["logInfo"] = [](const std::string& msg) { LOG_INFO(msg); };
        debug["logWarning"] = [](const std::string& msg) { LOG_WARN(msg); };
        debug["logError"] = [](const std::string& msg) { LOG_ERROR(msg); };
        debug["logDebug"] = [](const std::string& msg) { LOG_DEBUG(msg); };
    }

    bool LuaRuntime::executeFile(const std::string& path, sol::environment& environment) {
        if (!initialize()) return false;

        try {
            sol::load_result loadRes = m_state->load_file(path);
            if (!loadRes.valid()) {
                sol::error err = loadRes;
                reportError("LoadFile (" + path + ")", err);
                return false;
            }

            sol::protected_function scriptFunc = loadRes;
            sol::set_environment(environment, scriptFunc);

            sol::protected_function_result result = scriptFunc();
            if (!result.valid()) {
                sol::error err = result;
                reportError("Execution (" + path + ")", err);
                return false;
            }
            return true;
        }
        catch (const std::exception& ex) {
            LOG_ERROR("[Lua] Native exception during script execution: " + path + " - " + ex.what());
            return false;
        }
        catch (...) {
            LOG_ERROR("[Lua] Unknown exception during script execution: " + path);
            return false;
        }
    }

    void LuaRuntime::reportError(const std::string& source, const sol::error& error) const {
        LOG_ERROR("[Lua Error Traceback]\nLocation: " + source + "\nDetails:\n" + std::string(error.what()));
    }

    sol::state& LuaRuntime::state() {
        if (!initialize()) throw std::runtime_error("Lua runtime is not initialized");
        return *m_state;
    }
}