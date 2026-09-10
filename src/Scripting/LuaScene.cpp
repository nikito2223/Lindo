#include "LuaScene.h"
#include "LuaRuntime.h"
#include "Core/AssetManager.h"
#include "Core/Time/Time.h"
#include "Core/Input.h"
#include "Component/GameObject/GameObject.h"
#include "Component/PlayerController/Player.h"
#include "Debug/DebugLogger.h"

#include <chrono>
#include <filesystem>

namespace Lindo::Scripting {

    LuaScene::LuaScene(std::string scriptPath)
        : m_scriptPath(std::move(scriptPath)) {
    }

    void LuaScene::OnCreate() {
        auto startTime = std::chrono::high_resolution_clock::now();

        auto& runtime = LuaRuntime::Get();
        if (!runtime.initialize()) return;

        const std::string path = AssetManager::get().resolvePath(m_scriptPath, "scripts");
        LOG_INFO("[Lua Scene] Starting to load scene script: " + path);

        if (!std::filesystem::exists(path)) {
            LOG_ERROR("[Lua Scene] Scene script file missing: " + path);
            return;
        }

        m_environment = sol::environment(runtime.state(), sol::create, runtime.state().globals());
        m_environment["scene"] = this;
        m_environment["self"] = this;

        if (!runtime.executeFile(path, m_environment)) {
            LOG_ERROR("[Lua Scene] Failed to load scene script: " + path);
            return;
        }

        m_loaded = true;
        call("OnCreate");

        auto endTime = std::chrono::high_resolution_clock::now();
        auto durationMs = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();
        LOG_INFO("[Lua Scene] Scene successfully initialized in " + std::to_string(durationMs) + " ms: " + path);
    }

    void LuaScene::OnActivate() { call("OnActivate"); }
    void LuaScene::OnDeactivate() { call("OnDeactivate"); }

    void LuaScene::Update() {
        if (!m_loaded) return;

        for (const auto& object : GetGameObjects()) {
            if (object) object->Update();
        }

        try {
            sol::object value = m_environment["Update"];
            if (!value.is<sol::protected_function>()) return;

            sol::protected_function_result result = value.as<sol::protected_function>()(this, Time::GetDeltaTime());
            if (!result.valid()) {
                sol::error error = result;
                LuaRuntime::Get().reportError(m_scriptPath + "::Update", error);
            }
        }
        catch (const std::exception& e) {
            LOG_ERROR("[Lua Scene] Exception in Update (" + m_scriptPath + "): " + e.what());
        }
    }

    void LuaScene::ProcessInput(Lindo::Input::Input* input) {
        if (!m_loaded) return;

        try {
            sol::object value = m_environment["ProcessInput"];
            if (value.is<sol::protected_function>()) {
                sol::protected_function_result result = value.as<sol::protected_function>()(this, input);
                if (!result.valid()) {
                    sol::error error = result;
                    LuaRuntime::Get().reportError(m_scriptPath + "::ProcessInput", error);
                }
                return;
            }
        }
        catch (const std::exception& e) {
            LOG_ERROR("[Lua Scene] Exception in ProcessInput (" + m_scriptPath + "): " + e.what());
            return;
        }

        if (!input) return;
        auto* playerObject = FindGameObject("Player");
        auto* player = playerObject
            ? playerObject->getComponent<Lindo::Components::Controller::Player>()
            : nullptr;
        if (!player) return;

        const glm::vec2 mouseDelta = input->getMouseDelta();
        if (mouseDelta.x != 0.0f || mouseDelta.y != 0.0f) {
            player->Look(mouseDelta.x, mouseDelta.y);
            input->resetMouseDelta();
        }

        if (input->getActionDown("free_camera")) {
            player->SetFreeCameraMode(!player->IsFreeCameraMode());
        }
        if (player->IsFreeCameraMode()) return;

        player->MoveForward(input->getAxis("move_forward", "move_backward"));
        player->MoveRight(input->getAxis("move_right", "move_left"));
        player->SetCrouchInput(input->getAction("crouch"));

        if (input->getActionDown("jump")) player->Jump();
        if (input->getAction("sprint")) player->StartRunning();
        else player->StopRunning();
    }

    void LuaScene::OnDestroy() {
        if (m_loaded) call("OnDestroy");
        m_loaded = false;
    }

    void LuaScene::call(const char* name) {
        if (!m_loaded) return;

        try {
            sol::object value = m_environment[name];
            if (!value.is<sol::protected_function>()) return;

            sol::protected_function_result result = value.as<sol::protected_function>()(this);
            if (!result.valid()) {
                sol::error error = result;
                LuaRuntime::Get().reportError(m_scriptPath + "::" + name, error);
            }
        }
        catch (const std::exception& e) {
            LOG_ERROR("[Lua Scene] Exception in call(" + std::string(name) + ") on " + m_scriptPath + ": " + e.what());
        }
    }
}