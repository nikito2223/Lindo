#include "LuaScript.h"

#include "Core/Time/Time.h"
#include "Debug/DebugLogger.h"
#include "Scripting/LuaRuntime.h"
#include "Component/GameObject/GameObject.h"
#include "Core/AssetManager.h"

#include <utility>
#include <chrono>
#include <filesystem>

namespace Lindo::Scripting {

    LuaScript::LuaScript(std::string scriptPath)
        : m_scriptPath(std::move(scriptPath)) {
    }

    void LuaScript::setScriptPath(const std::string& path) {
        m_scriptPath = path;
        m_loaded = false;
        m_started = false;
    }

    void LuaScript::OnStart() {
        if (m_scriptPath.empty() || !gameObject) return;

        auto startTime = std::chrono::high_resolution_clock::now();

        auto& runtime = LuaRuntime::Get();
        if (!runtime.initialize()) {
            LOG_ERROR("[Lua] Runtime initialization failed when loading: " + m_scriptPath);
            return;
        }

        std::string fullPath = AssetManager::get().resolvePath(m_scriptPath, "scripts");
        LOG_INFO("[Lua] Attempting to load script: " + fullPath);

        if (!std::filesystem::exists(fullPath)) {
            LOG_ERROR("[Lua] Script file does not exist: " + fullPath);
            return;
        }

        m_environment = sol::environment(runtime.state(), sol::create, runtime.state().globals());
        m_environment["gameObject"] = gameObject;
        m_environment["self"] = gameObject;

        if (!runtime.executeFile(fullPath, m_environment)) {
            LOG_ERROR("[Lua] Failed to execute script: " + fullPath);
            return;
        }

        m_loaded = true;

        call("Awake");
        call("Start");
        m_started = true;

        auto endTime = std::chrono::high_resolution_clock::now();
        auto durationMs = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();
        LOG_INFO("[Lua] Script loaded and initialized successfully in " + std::to_string(durationMs) + " ms: " + fullPath);
    }

    void LuaScript::OnUpdate() {
        if (!m_loaded || !m_started || !m_enabled) return;
        callUpdate();
    }

    void LuaScript::OnDestroy() {
        if (m_loaded) call("OnDestroy");
        m_loaded = false;
        m_started = false;
    }

    void LuaScript::reload() {
        m_loaded = false;
        m_started = false;
        OnStart();
    }

    void LuaScript::call(const char* functionName) {
        if (!m_loaded) return;

        try {
            sol::object functionObject = m_environment[functionName];
            if (!functionObject.is<sol::protected_function>()) return;

            sol::protected_function function = functionObject.as<sol::protected_function>();
            sol::protected_function_result result = function(m_environment["self"]);

            if (!result.valid()) {
                sol::error err = result;
                LuaRuntime::Get().reportError(m_scriptPath + "::" + functionName, err);
            }
        }
        catch (const std::exception& e) {
            LOG_ERROR("[Lua] Native exception during call to " + m_scriptPath + "::" + functionName + ": " + e.what());
        }
        catch (...) {
            LOG_ERROR("[Lua] Unknown native exception during call to " + m_scriptPath + "::" + functionName);
        }
    }

    void LuaScript::callUpdate() {
        if (!m_loaded) return;

        try {
            sol::object functionObject = m_environment["Update"];
            if (!functionObject.is<sol::protected_function>()) return;

            sol::protected_function function = functionObject.as<sol::protected_function>();
            sol::protected_function_result result = function(m_environment["self"], Time::GetDeltaTime());

            if (!result.valid()) {
                sol::error err = result;
                LuaRuntime::Get().reportError(m_scriptPath + "::Update", err);
            }
        }
        catch (const std::exception& e) {
            LOG_ERROR("[Lua] Native exception during Update in " + m_scriptPath + ": " + e.what());
        }
        catch (...) {
            LOG_ERROR("[Lua] Unknown native exception during Update in " + m_scriptPath);
        }
    }
}