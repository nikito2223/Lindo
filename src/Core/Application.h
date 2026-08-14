#pragma once
#include <memory>
#include <string>
#include "Input.h"
#include "SceneManager.h"
#include "../Graphics/ui/UIManager.h"
#include "../Graphics/core/Renderer.h"
#include "../Window/Window.h"
#include "../debug/DebugOverlay.h"
#include "Debug/DebugLogger.h"

namespace Lindo {

    // === Метаданные приложения / движка ===
    struct AppInfo {
        static constexpr const char* Name = "Lindo";
        static constexpr int VersionMajor = 26;
        static constexpr int VersionMinor = 0;
        static constexpr int VersionPatch = 8;
        static constexpr const char* Stage = "dev"; // dev, alpha, beta, release
        
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

    private:
        std::unique_ptr<Lindo::Window> m_window;
        std::unique_ptr<Lindo::Input::Input> m_input;
        Lindo::SceneManager* m_sceneManager;
        std::unique_ptr<Lindo::Graphics::UI::UIManager> m_uiManager;
        std::unique_ptr<Lindo::Core::DebugLogger> m_debugLogger;
        std::unique_ptr<Lindo::Debug::DebugOverlay> m_debugOverlay;
        std::unique_ptr<Lindo::Graphics::Renderer> m_renderer;

        float m_lastFrameTime = 0.0f;
    };
}