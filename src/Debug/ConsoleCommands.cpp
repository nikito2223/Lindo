#include "ConsoleCommands.h"
#include "Core/system/EngineContext.h"
#include "Window/Window.h"
#include <debug/Console.h>
#include "Debug/DebugSystem.h"
#include "Debug/DebugOverlay.h"
#include "Core/SceneManager.h"
#include "Debug/DebugLogger.h"
#include "core/Application.h"

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

        // --- ¬строенные команды (перенесены из Console.cpp) ---
        console->setGLFWWindow(window->getHandle());
        console->registerCommand("help", "Show a list of available commands", [console](const std::vector<std::string>&) {
            // “.к.пр€мого доступа к m_commands нет, встроенные команды лучше регистрировать здесь 
            // или сделать метод получени€ команд. Ќо пока зарегистрируем стандартные базовые:
            console->print("Available built-in commands:", Lindo::Graphics::UI::Color(0.6f, 0.9f, 1.0f, 1.0f));
            console->print("  help - Show this help message", Lindo::Graphics::UI::Color(0.8f, 0.8f, 0.8f, 1.0f));
            console->print("  clear - Clear console log", Lindo::Graphics::UI::Color(0.8f, 0.8f, 0.8f, 1.0f));
            console->print("  echo <text> - Print text", Lindo::Graphics::UI::Color(0.8f, 0.8f, 0.8f, 1.0f));
            console->print("  history - Show command history", Lindo::Graphics::UI::Color(0.8f, 0.8f, 0.8f, 1.0f));
            console->print("  exit - Close application", Lindo::Graphics::UI::Color(0.8f, 0.8f, 0.8f, 1.0f));
            console->print("  fullscreen - Toggle fullscreen mode", Lindo::Graphics::UI::Color(0.8f, 0.8f, 0.8f, 1.0f));
            console->print("  debug_overlay - Toggle debug overlay", Lindo::Graphics::UI::Color(0.8f, 0.8f, 0.8f, 1.0f));
            console->print("  scene_load <name> - Load a scene", Lindo::Graphics::UI::Color(0.8f, 0.8f, 0.8f, 1.0f));
            console->print("  version - Show engine version", Lindo::Graphics::UI::Color(0.8f, 0.8f, 0.8f, 1.0f));
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
            // ≈сли у консоли есть метод получени€ истории, используйте его. 
            // Ћибо оставьте вывод истории здесь, если есть доступ, или выведите заглушку.
            console->print("Command history feature.", Lindo::Graphics::UI::Color(0.6f, 0.6f, 0.6f, 1.0f));
            });

        // --- ѕользовательские команды приложени€ ---

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
                context->getSceneManager()->LoadScene(args[0]);
            }
            });

        console->registerCommand("version", "Show engine version", [](const std::vector<std::string>&) {
            LOG_INFO(std::string(AppInfo::Name) + " v" + AppInfo::GetVersionString());
            });
    }

}