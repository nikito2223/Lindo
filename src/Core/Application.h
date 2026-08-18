#pragma once
#include <memory>
#include <string>
#include "system/EngineContext.h"
#include "../Window/Window.h"
#include "Debug/DebugLogger.h"

namespace Lindo {

    struct AppInfo {
        static constexpr const char* Name = "Lindo";
        static constexpr int VersionMajor = 26;
        static constexpr int VersionMinor = 1;
        static constexpr int VersionPatch = 8;
        static constexpr const char* Stage = "dev";

        static std::string GetVersionString() {
            return std::to_string(VersionMajor) + "." +
                std::to_string(VersionMinor) + "." +
                std::to_string(VersionPatch) + "-" + Stage;
        }

        static std::string GetFormattedTitle() {
            std::string title = std::string(Name) + " v" + GetVersionString();
#ifdef _DEBUG
            title += " [Debug]";
#else
            title += " [Release]";
#endif
            return title;
        }
    };

    class Application {
    public:
        Application();
        ~Application();

        void run();

    private:
        void logAppHeader();
        void logGPUInfo();
        void registerConsoleCommands();

    private:
        std::unique_ptr<Lindo::Window> m_window;
        std::unique_ptr<Lindo::EngineContext> m_context;
    };
}