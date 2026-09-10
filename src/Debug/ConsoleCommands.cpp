#include "ConsoleCommands.h"
#include "Core/system/EngineContext.h"
#include "Window/Window.h"
#include <debug/Console.h>
#include "Debug/DebugSystem.h"
#include "Debug/DebugOverlay.h"
#include "Core/SceneManager.h"
#include "Debug/DebugLogger.h"
#include "core/Application.h"
#include "Core/Types/Settings.h" // Подключаем ваши настройки
#include "Core/Time/Time.h"

#include <GLFW/glfw3.h>
#include <vector>
#include <string>
#include <algorithm>

namespace Lindo {

    void ConsoleCommands::RegisterAll(EngineContext* context, Window* window) {
        if (!context) return;

        auto* ui = context->getUI();
        if (!ui) return;

        Lindo::Debug::Console* console = ui->getConsole();
        if (!console) return;

        // --- Встроенные команды ---
        console->setGLFWWindow(window->getHandle());
        console->registerCommand("help", "Show a list of available commands", [console](const std::vector<std::string>&) {
            console->print("--- Built-in Commands ---", Lindo::Graphics::UI::Color(0.6f, 0.9f, 1.0f, 1.0f));
            console->print("  help - Show this help message", Lindo::Graphics::UI::Color(0.8f, 0.8f, 0.8f, 1.0f));
            console->print("  clear - Clear console log", Lindo::Graphics::UI::Color(0.8f, 0.8f, 0.8f, 1.0f));
            console->print("  echo <text> - Print text", Lindo::Graphics::UI::Color(0.8f, 0.8f, 0.8f, 1.0f));
            console->print("  history - Show command history", Lindo::Graphics::UI::Color(0.8f, 0.8f, 0.8f, 1.0f));
            console->print("  exit - Close application", Lindo::Graphics::UI::Color(0.8f, 0.8f, 0.8f, 1.0f));
            console->print("  fullscreen - Toggle fullscreen mode", Lindo::Graphics::UI::Color(0.8f, 0.8f, 0.8f, 1.0f));
            console->print("  debug_overlay - Toggle debug overlay", Lindo::Graphics::UI::Color(0.8f, 0.8f, 0.8f, 1.0f));
            console->print("  scene_load <name> - Load a scene", Lindo::Graphics::UI::Color(0.8f, 0.8f, 0.8f, 1.0f));
            console->print("  scene_list - List registered scenes", Lindo::Graphics::UI::Color(0.8f, 0.8f, 0.8f, 1.0f));
            console->print("  scene_current - Show current scene", Lindo::Graphics::UI::Color(0.8f, 0.8f, 0.8f, 1.0f));
            console->print("  pause [0|1] - Pause/resume game time", Lindo::Graphics::UI::Color(0.8f, 0.8f, 0.8f, 1.0f));
            console->print("  version - Show engine version", Lindo::Graphics::UI::Color(0.8f, 0.8f, 0.8f, 1.0f));

            console->print("--- Render Settings (r_) ---", Lindo::Graphics::UI::Color(0.6f, 0.9f, 1.0f, 1.0f));
            console->print("  r_fov <float> - Get/Set FOV angle (10-170)", Lindo::Graphics::UI::Color(0.8f, 0.8f, 0.8f, 1.0f));
            console->print("  r_gamma <float> - Get/Set Gamma value", Lindo::Graphics::UI::Color(0.8f, 0.8f, 0.8f, 1.0f));
            console->print("  r_exposure <float> - Get/Set Exposure value", Lindo::Graphics::UI::Color(0.8f, 0.8f, 0.8f, 1.0f));
            console->print("  r_bloom <0|1> - Enable/Disable Bloom", Lindo::Graphics::UI::Color(0.8f, 0.8f, 0.8f, 1.0f));
            console->print("  r_fxaa <0|1> - Enable/Disable FXAA", Lindo::Graphics::UI::Color(0.8f, 0.8f, 0.8f, 1.0f));
            console->print("  r_ssao <0|1> - Enable/Disable SSAO", Lindo::Graphics::UI::Color(0.8f, 0.8f, 0.8f, 1.0f));
            console->print("  r_shadows <0|1> - Enable/Disable Shadows", Lindo::Graphics::UI::Color(0.8f, 0.8f, 0.8f, 1.0f));
            console->print("  r_shadow_quality <0-4> - Set Shadow Quality", Lindo::Graphics::UI::Color(0.8f, 0.8f, 0.8f, 1.0f));
            console->print("  r_msaa <int> - Set MSAA samples count", Lindo::Graphics::UI::Color(0.8f, 0.8f, 0.8f, 1.0f));
            console->print("  r_wireframe <0|1> - Enable/Disable Wireframe mode", Lindo::Graphics::UI::Color(0.8f, 0.8f, 0.8f, 1.0f));

            console->print("--- Display Settings (vid_) ---", Lindo::Graphics::UI::Color(0.6f, 0.9f, 1.0f, 1.0f));
            console->print("  vid_vsync <0|1> - Enable/Disable Vertical Sync", Lindo::Graphics::UI::Color(0.8f, 0.8f, 0.8f, 1.0f));
            console->print("  vid_maxfps <int> - Set Target FPS limit", Lindo::Graphics::UI::Color(0.8f, 0.8f, 0.8f, 1.0f));

            console->print("--- Sound Settings (s_) ---", Lindo::Graphics::UI::Color(0.6f, 0.9f, 1.0f, 1.0f));
            console->print("  s_volume <float> - Get/Set Master Volume (0.0 to 1.0)", Lindo::Graphics::UI::Color(0.8f, 0.8f, 0.8f, 1.0f));
            console->print("  s_musicvolume <float> - Get/Set Music Volume (0.0 to 1.0)", Lindo::Graphics::UI::Color(0.8f, 0.8f, 0.8f, 1.0f));
            console->print("  s_sfxvolume <float> - Get/Set SFX Volume (0.0 to 1.0)", Lindo::Graphics::UI::Color(0.8f, 0.8f, 0.8f, 1.0f));
            console->print("  s_mute <0|1> - Mute/Unmute all audio", Lindo::Graphics::UI::Color(0.8f, 0.8f, 0.8f, 1.0f));

            console->print("--- System & Debug (cl_ / sys_) ---", Lindo::Graphics::UI::Color(0.6f, 0.9f, 1.0f, 1.0f));
            console->print("  cl_showfps <0|1> - Show/Hide FPS counter", Lindo::Graphics::UI::Color(0.8f, 0.8f, 0.8f, 1.0f));
            console->print("  sys_debugmode <0|1> - Enable/Disable Engine Debug Mode", Lindo::Graphics::UI::Color(0.8f, 0.8f, 0.8f, 1.0f));
            console->print("  saveconfig - Save current settings to ini files", Lindo::Graphics::UI::Color(0.8f, 0.8f, 0.8f, 1.0f));
            console->print("  resetconfig - Reset settings to defaults", Lindo::Graphics::UI::Color(0.8f, 0.8f, 0.8f, 1.0f));
        });

        console->registerCommand("clear", "Clear console log", [console](const std::vector<std::string>&) {
            console->clear();
        });

        console->registerCommand("echo", "Print passed text", [console](const std::vector<std::string>& args) {
            std::string joined;
            for (size_t i = 0; i < args.size(); ++i) {
                if (i) joined += " ";
                joined += args[i];
            }
            console->print(joined);
        });

        console->registerCommand("history", "Show entered commands history", [console](const std::vector<std::string>&) {
            console->print("Command history feature.", Lindo::Graphics::UI::Color(0.6f, 0.6f, 0.6f, 1.0f));
        });

        if (window) {
            console->registerCommand("exit", "Close application", [window](const std::vector<std::string>&) {
                glfwSetWindowShouldClose(window->getHandle(), GLFW_TRUE);
            });

            console->registerCommand("fullscreen", "Toggle fullscreen mode (F11)", [window](const std::vector<std::string>&) {
                window->toggleFullscreen();
            });
        }

        console->registerCommand("debug_overlay", "Show/hide debug overlay (F3)", [context](const std::vector<std::string>&) {
            if (context->getDebugSystem()) {
                bool state = context->getDebugSystem()->getOverlay()->toggle();
                LOG_INFO(std::string("[Console] Debug overlay: ") + (state ? "ON" : "OFF"));
            }
        });

        console->registerCommand("scene_load", "scene_load <name> - load a scene", [context](const std::vector<std::string>& args) {
            if (!args.empty() && context->getSceneManager()) {
                context->getSceneManager()->RequestLoadScene(args[0]);
            }
        });

        console->registerCommand("scene_list", "List registered scenes", [context, console](const std::vector<std::string>&) {
            auto* scenes = context->getSceneManager();
            if (!scenes) return;
            const std::string& current = scenes->GetCurrentSceneName();
            for (const auto& name : scenes->GetRegisteredSceneNames()) {
                console->print((name == current ? "* " : "  ") + name);
            }
        });

        console->registerCommand("scene_current", "Show current scene", [context, console](const std::vector<std::string>&) {
            const auto* scenes = context->getSceneManager();
            console->print(scenes && !scenes->GetCurrentSceneName().empty()
                ? "Current scene: " + scenes->GetCurrentSceneName()
                : "No active scene");
        });

        console->registerCommand("pause", "pause [0|1] - pause/resume game time", [console](const std::vector<std::string>& args) {
            bool paused = Lindo::Time::GetTimeScale() == 0.0f;
            if (!args.empty()) paused = args[0] == "1" || args[0] == "true" || args[0] == "on";
            Lindo::Time::SetTimeScale(paused ? 0.0f : 1.0f);
            console->print(paused ? "Game paused" : "Game resumed");
        });

        console->registerCommand("version", "Show engine version", [](const std::vector<std::string>&) {
            LOG_INFO(std::string(AppInfo::Name) + " v" + AppInfo::GetVersionString());
        });

        // Регистрируем настройки
        RegisterSettingsCommands(context, window);
    }

    void ConsoleCommands::RegisterSettingsCommands(EngineContext* context, Window* window) {
        auto* console = context->getUI()->getConsole();

        // -------------------------------------------------------------
        // --- РЕНДЕРИНГ И ГРАФИКА (Префикс r_) ---
        // -------------------------------------------------------------

        console->registerCommand("r_fov", "Get/Set FOV angle (10-170)", [console](const std::vector<std::string>& args) {
            auto& settings = Settings::getInstance();
            if (args.empty()) {
                console->print("r_fov is " + std::to_string(settings.fov));
                
                return;
            }
            try {
                float val = std::stof(args[0]);
                settings.fov = std::clamp(val, 10.0f, 170.0f);
                console->print("r_fov set to " + std::to_string(settings.fov));
                settings.apply();
            } catch (...) { console->print("Usage: r_fov <float>"); }
        });

        console->registerCommand("r_gamma", "Get/Set Gamma value", [console](const std::vector<std::string>& args) {
            auto& settings = Settings::getInstance();
            if (args.empty()) {
                console->print("r_gamma is " + std::to_string(settings.gamma));
                return;
            }
            try {
                settings.gamma = std::stof(args[0]);
                settings.apply();
                console->print("r_gamma set to " + std::to_string(settings.gamma));
            } catch (...) { console->print("Usage: r_gamma <float>"); }
        });

        console->registerCommand("r_exposure", "Get/Set Exposure value", [console](const std::vector<std::string>& args) {
            auto& settings = Settings::getInstance();
            if (args.empty()) {
                console->print("r_exposure is " + std::to_string(settings.exposure));
                return;
            }
            try {
                settings.exposure = std::stof(args[0]);
                settings.apply();
                console->print("r_exposure set to " + std::to_string(settings.exposure));
            } catch (...) { console->print("Usage: r_exposure <float>"); }
        });

        console->registerCommand("r_bloom", "Enable/Disable Bloom (0 or 1)", [console](const std::vector<std::string>& args) {
            auto& settings = Settings::getInstance();
            if (args.empty()) {
                console->print("r_bloom is " + std::to_string(settings.enableBloom));
                return;
            }
            settings.enableBloom = (args[0] == "1" || args[0] == "true");
            settings.apply();
            console->print("r_bloom set to " + std::to_string(settings.enableBloom));
        });

        console->registerCommand("r_fxaa", "Enable/Disable FXAA (0 or 1)", [console](const std::vector<std::string>& args) {
            auto& settings = Settings::getInstance();
            if (args.empty()) {
                console->print("r_fxaa is " + std::to_string(settings.enableFXAA));
                return;
            }
            settings.enableFXAA = (args[0] == "1" || args[0] == "true");
            settings.apply();
            console->print("r_fxaa set to " + std::to_string(settings.enableFXAA));
        });

        console->registerCommand("r_ssao", "Enable/Disable SSAO (0 or 1)", [console](const std::vector<std::string>& args) {
            auto& settings = Settings::getInstance();
            if (args.empty()) {
                console->print("r_ssao is " + std::to_string(settings.enableSSAO));
                return;
            }
            settings.enableSSAO = (args[0] == "1" || args[0] == "true");
            settings.apply();
            console->print("r_ssao set to " + std::to_string(settings.enableSSAO));
        });

        console->registerCommand("r_shadows", "Enable/Disable Shadows (0 or 1)", [console](const std::vector<std::string>& args) {
            auto& settings = Settings::getInstance();
            if (args.empty()) {
                console->print("r_shadows is " + std::to_string(settings.enableShadows));
                return;
            }
            settings.enableShadows = (args[0] == "1" || args[0] == "true");
            settings.apply();
            console->print("r_shadows set to " + std::to_string(settings.enableShadows));
        });

        console->registerCommand("r_shadow_quality", "Set Shadow Quality (0:Off, 1:Low, 2:Med, 3:High, 4:Ultra)", [console](const std::vector<std::string>& args) {
            auto& settings = Settings::getInstance();
            if (args.empty()) {
                console->print("r_shadow_quality is " + std::to_string(static_cast<int>(settings.shadowQuality)));
                return;
            }
            try {
                int val = std::clamp(std::stoi(args[0]), 0, 4);
                settings.shadowQuality = static_cast<ShadowQuality>(val);
                settings.apply();
                console->print("r_shadow_quality set to " + std::to_string(val));
            } catch (...) { console->print("Usage: r_shadow_quality <0-4>"); }
        });

        console->registerCommand("r_msaa", "Set MSAA samples count (1, 2, 4, 8)", [console](const std::vector<std::string>& args) {
            auto& settings = Settings::getInstance();
            if (args.empty()) {
                console->print("r_msaa is " + std::to_string(settings.msaaSamples));
                return;
            }
            try {
                settings.msaaSamples = std::stoi(args[0]);
                settings.apply();
                console->print("r_msaa set to " + std::to_string(settings.msaaSamples));
            } catch (...) { console->print("Usage: r_msaa <int>"); }
        });

        console->registerCommand("r_wireframe", "Enable/Disable Wireframe mode (0 or 1)", [console](const std::vector<std::string>& args) {
            auto& settings = Settings::getInstance();
            if (args.empty()) {
                console->print("r_wireframe is " + std::to_string(settings.wireframeMode));
                return;
            }
            settings.wireframeMode = (args[0] == "1" || args[0] == "true");
            settings.apply();
            console->print("r_wireframe set to " + std::to_string(settings.wireframeMode));
        });

        // -------------------------------------------------------------
        // --- ДИСПЛЕЙ И ОКНО (Префикс vid_) ---
        // -------------------------------------------------------------

        console->registerCommand("vid_vsync", "Enable/Disable Vertical Sync (0 or 1)", [console](const std::vector<std::string>& args) {
            auto& display = DisplaySettings::getInstance();
            if (args.empty()) {
                console->print("vid_vsync is " + std::to_string(display.vsync));
                return;
            }
            display.vsync = (args[0] == "1" || args[0] == "true");
            display.apply();
            console->print("vid_vsync set to " + std::to_string(display.vsync));
        });

        console->registerCommand("vid_maxfps", "Set Target FPS limit (0 = uncapped)", [console](const std::vector<std::string>& args) {
            auto& display = DisplaySettings::getInstance();
            if (args.empty()) {
                console->print("vid_maxfps is " + std::to_string(display.targetFPS));
                return;
            }
            try {
                display.targetFPS = std::stoi(args[0]);
                display.apply();
                console->print("vid_maxfps set to " + std::to_string(display.targetFPS));
            } catch (...) { console->print("Usage: vid_maxfps <int>"); }
        });

        // -------------------------------------------------------------
        // --- АУДИО (Префикс s_ или snd_) ---
        // -------------------------------------------------------------

        console->registerCommand("s_volume", "Get/Set Master Volume (0.0 to 1.0)", [console](const std::vector<std::string>& args) {
            auto& settings = Settings::getInstance();
            if (args.empty()) {
                console->print("s_volume is " + std::to_string(settings.masterVolume));
                return;
            }
            try {
                float val = std::stof(args[0]);
                settings.masterVolume = std::clamp(val, 0.0f, 1.0f);
                settings.apply();
                console->print("s_volume set to " + std::to_string(settings.masterVolume));
            } catch (...) { console->print("Usage: s_volume <0.0-1.0>"); }
        });

        console->registerCommand("s_musicvolume", "Get/Set Music Volume (0.0 to 1.0)", [console](const std::vector<std::string>& args) {
            auto& settings = Settings::getInstance();
            if (args.empty()) {
                console->print("s_musicvolume is " + std::to_string(settings.musicVolume));
                return;
            }
            try {
                float val = std::stof(args[0]);
                settings.musicVolume = std::clamp(val, 0.0f, 1.0f);
                settings.apply();
                console->print("s_musicvolume set to " + std::to_string(settings.musicVolume));
            } catch (...) { console->print("Usage: s_musicvolume <0.0-1.0>"); }
        });

        console->registerCommand("s_sfxvolume", "Get/Set SFX Volume (0.0 to 1.0)", [console](const std::vector<std::string>& args) {
            auto& settings = Settings::getInstance();
            if (args.empty()) {
                console->print("s_sfxvolume is " + std::to_string(settings.sfxVolume));
                return;
            }
            try {
                float val = std::stof(args[0]);
                settings.sfxVolume = std::clamp(val, 0.0f, 1.0f);
                settings.apply();
                console->print("s_sfxvolume set to " + std::to_string(settings.sfxVolume));
            } catch (...) { console->print("Usage: s_sfxvolume <0.0-1.0>"); }
        });

        console->registerCommand("s_mute", "Mute/Unmute all audio (0 or 1)", [console](const std::vector<std::string>& args) {
            auto& settings = Settings::getInstance();
            if (args.empty()) {
                console->print("s_mute is " + std::to_string(settings.muteAudio));
                return;
            }
            settings.muteAudio = (args[0] == "1" || args[0] == "true");
            settings.apply();
            console->print("s_mute set to " + std::to_string(settings.muteAudio));
        });

        // -------------------------------------------------------------
        // --- ОТЛАДКА И КЛИЕНТ (Префикс cl_ / sys_) ---
        // -------------------------------------------------------------

        console->registerCommand("cl_showfps", "Show/Hide FPS counter (0 or 1)", [console](const std::vector<std::string>& args) {
            auto& settings = Settings::getInstance();
            if (args.empty()) {
                console->print("cl_showfps is " + std::to_string(settings.showFPS));
                return;
            }
            settings.showFPS = (args[0] == "1" || args[0] == "true");
            settings.apply();
            console->print("cl_showfps set to " + std::to_string(settings.showFPS));
        });

        console->registerCommand("sys_debugmode", "Enable/Disable Engine Debug Mode (0 or 1)", [console](const std::vector<std::string>& args) {
            auto& settings = Settings::getInstance();
            if (args.empty()) {
                console->print("sys_debugmode is " + std::to_string(settings.debugMode));
                return;
            }
            settings.debugMode = (args[0] == "1" || args[0] == "true");
            settings.apply();
            console->print("sys_debugmode set to " + std::to_string(settings.debugMode));
        });

        // -------------------------------------------------------------
        // --- СИСТЕМНЫЕ УТИЛИТЫ ---
        // -------------------------------------------------------------

        console->registerCommand("saveconfig", "Save current settings to config files", [console](const std::vector<std::string>&) {
            Settings::getInstance().saveToFile();
            DisplaySettings::getInstance().saveToFile();
            console->print("Settings saved successfully.");
        });

        console->registerCommand("resetconfig", "Reset settings to defaults", [console](const std::vector<std::string>&) {
            Settings::getInstance().resetToDefaults();
            console->print("Settings reset to default values.");
        });
    }

}