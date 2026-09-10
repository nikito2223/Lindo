#include "Application.h"
#include "Window/Window.h"
#include <debug/Console.h>
#include <Physics/PhysicsSystem.h>
#include <Component/Audio/AudioSystem.h>
#include "Types/Settings.h"
#include "Core/OGL.h"
#include "Core/Time/Time.h"
#include "Core/FrameManager.h"
#include "Core/Memory/Memory.h"
#include "Core/LindoCrashHandle.h"

#include "Debug/DebugSystem.h"
#include "Debug/DebugOverlay.h"
#include "Debug/ConsoleCommands.h"

#include "Graphics/core/Framebuffer.h"
#include "Component/Camera/Camera.h"
#include "world/Scene.h"

#include <algorithm>
#include <thread>
#include <GLFW/glfw3.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

/**
 * @brief Экспорт флагов для принудительного запуска приложения на дискретных видеокартах NVIDIA / AMD.
 */
extern "C" {
    __declspec(dllexport) unsigned long NvOptimusEnablement = 0x00000001;
    __declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}

namespace {
    /**
     * @brief Отправляет системный сигнал о завершении работы сплэш-экрана на Windows.
     */
    void SignalSplashFinished() {
        const wchar_t* EVENT_NAME = L"Global\\LindoSplash";
        HANDLE hEvent = OpenEventW(EVENT_MODIFY_STATE, FALSE, EVENT_NAME);
        if (hEvent) {
            SetEvent(hEvent);
            CloseHandle(hEvent);
        }
    }
}
#endif

namespace Lindo {

    Application::Application() {
        Lindo::Core::DebugLogger::Init("engine_log.txt");
        try {
            logAppHeader();

            LOG_INFO("[Application] Creating application window...");
            DisplaySettings& displaySettings = DisplaySettings::getInstance();

            m_window = std::make_unique<Lindo::Window>(
                displaySettings.windowWidth,
                displaySettings.windowHeight,
                AppInfo::GetFormattedTitle().c_str()
            );

            m_context = std::make_unique<Lindo::EngineContext>();
        }
        catch (...) {
            Lindo::Core::DebugLogger::Close();
            throw;
        }
    }

    Application::~Application() {
        m_context.reset();
        m_window.reset();
        Lindo::Core::Memory::Get().report("Application shutdown");
        LOG_INFO("==================================================");
        LOG_INFO(std::string(AppInfo::Name) + " shutting down...");
        LOG_INFO("==================================================");
        Lindo::Core::LindoCrashHandle::Shutdown();
        Lindo::Core::DebugLogger::Close();
    }

    void Application::logAppHeader() {
        LOG_INFO("==================================================");
        LOG_INFO("  _        _             ");
        LOG_INFO(" | |  _   | | ___  ___   ");
        LOG_INFO(" | | | | _| |/ _ \\/ _ \\  ");
        LOG_INFO(" | |_| |/ |_|  __/ (_) | ");
        LOG_INFO(" |____/\\__,_|\\___|\\___/  ");
        LOG_INFO("==================================================");
        LOG_INFO(" Engine:   " + std::string(AppInfo::Name));
        LOG_INFO(" Version:  " + AppInfo::GetVersionString());
        LOG_INFO(" Build:    " + std::string(__DATE__) + " " + std::string(__TIME__));
#ifdef _DEBUG
        LOG_INFO(" Mode:     Debug");
#else
        LOG_INFO(" Mode:     Release");
#endif
        LOG_INFO("==================================================");
    }

    void Application::logGPUInfo() {
        const char* vendorRaw = reinterpret_cast<const char*>(glGetString(GL_VENDOR));
        const char* rendererRaw = reinterpret_cast<const char*>(glGetString(GL_RENDERER));
        const char* versionRaw = reinterpret_cast<const char*>(glGetString(GL_VERSION));

        std::string vendor = vendorRaw ? vendorRaw : "Unknown Vendor";
        std::string renderer = rendererRaw ? rendererRaw : "Unknown GPU";
        std::string version = versionRaw ? versionRaw : "Unknown GL Version";

        LOG_INFO("==================================================");
        LOG_INFO(" GPU Vendor:   " + vendor);
        LOG_INFO(" GPU Model:    " + renderer);
        LOG_INFO(" OpenGL Ver:   " + version);
        LOG_INFO("--------------------------------------------------");
    }

    void Application::run() {
        if (m_window && m_window->getNativeWindow()) {
            glfwShowWindow(m_window->getNativeWindow());
            glfwPollEvents();
            
            // Очищаем экран первым кадром (темно-серый)
            glClearColor(0.08f, 0.08f, 0.1f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);
            glfwSwapBuffers(m_window->getNativeWindow());
        }

        LOG_INFO("[Application] Starting Context Initialization...");
        
        // Запуск контекста
        if (!m_context->init(m_window.get())) {
            throw std::runtime_error("Engine context initialization failed");
        }

        Lindo::ConsoleCommands::RegisterAll(m_context.get(), m_window.get());
        logGPUInfo();

#ifdef _WIN32
        SignalSplashFinished();
        if (m_window && m_window->getNativeWindow()) {
            glfwFocusWindow(m_window->getNativeWindow());
        }
#endif

        LOG_INFO("[Application] Entering main loop...");

        DisplaySettings& displaySettings = DisplaySettings::getInstance();
        Lindo::FrameManager::Init(displaySettings.targetFPS);
        displaySettings.addOnChangedCallback([this](const DisplaySettings& changedSettings) {
            if (m_window) {
                m_window->setVSync(changedSettings.vsync);
            }
            Lindo::FrameManager::SetTargetFPS(changedSettings.targetFPS);
        });

        auto& settings = Settings::getInstance();
        settings.addOnChangedCallback([this](const Settings& changedSettings) {
            if (m_context && m_context->getRenderer()) {
                m_context->getRenderer()->applySettings(changedSettings);
            }

            auto* sceneManager = m_context ? m_context->getSceneManager() : nullptr;
            auto* scene = sceneManager ? sceneManager->GetCurrentScene() : nullptr;
            if (!scene) return;

            for (auto* camera : scene->FindComponentsOfType<Lindo::Components::Rendering::Camera>()) {
                if (camera) camera->applySettings(changedSettings);
            }
        });

        auto* input = m_context->getInput();
        auto* ui = m_context->getUI();
        auto* renderer = m_context->getRenderer();
        auto* debugSystem = m_context->getDebugSystem();
        auto* sceneManager = m_context->getSceneManager();

        // ROOT CAUSE FIX: Renderer caches its own viewport size and only
        // updates it when the GLFW resize callback chain actually fires.
        // That chain is not reliable in every situation - e.g. a freshly
        // created/shown window not yet reporting its true framebuffer size
        // until it receives focus, or glfwSetWindowMonitor() (used when
        // toggling fullscreen) not always delivering its resize callback
        // synchronously on every platform/driver. When the callback is
        // missed, the renderer keeps rendering at a stale size forever,
        // showing up as black/grey bars on startup (until Alt-Tab forces a
        // focus event) or a viewport permanently "stuck" after leaving
        // fullscreen.
        //
        // Rather than depend on that event ever firing, poll the real,
        // current framebuffer size once per frame and resync the renderer
        // immediately whenever it's changed. This makes the renderer
        // self-healing regardless of why an event was missed.
        int lastKnownFbWidth = 0;
        int lastKnownFbHeight = 0;
        if (m_window && m_window->getNativeWindow()) {
            glfwGetFramebufferSize(m_window->getNativeWindow(), &lastKnownFbWidth, &lastKnownFbHeight);
            if (renderer) {
                renderer->onResize(lastKnownFbWidth, lastKnownFbHeight);
            }
        }

        while (!m_window->shouldClose()) {
            Lindo::FrameManager::BeginFrame();
            Lindo::Time::Update();

            input->update(m_window.get());
            m_window->pollEvents();

            if (renderer && m_window && m_window->getNativeWindow()) {
                int fbWidth = 0, fbHeight = 0;
                glfwGetFramebufferSize(m_window->getNativeWindow(), &fbWidth, &fbHeight);
                if (fbWidth > 0 && fbHeight > 0 &&
                    (fbWidth != lastKnownFbWidth || fbHeight != lastKnownFbHeight)) {
                    renderer->onResize(fbWidth, fbHeight);
                    lastKnownFbWidth = fbWidth;
                    lastKnownFbHeight = fbHeight;
                }
            }

            if (input->consumeF3()) {
                debugSystem->getOverlay()->toggle();
            }

            if (input->consumeF4()) {
                auto& settings = Lindo::Settings::getInstance();
                bool newState = !settings.isDebugDrawEnabled();
                settings.setDebugDrawEnabled(newState);

                LOG_INFO(std::string("[Input] Physics debug draw toggled: ") + (newState ? "ON" : "OFF"));
            }

            if (input->consumeF11()) {
                m_window->toggleFullscreen();
            }

            if (input->consumeKey(Lindo::Input::KeyCode::F6)) {
                if (auto* editor = m_context->getEditor()) editor->toggle();
            }

            //if (input->consumeEscape()) {
            //    input->setUIActive(!input->isUIActive());
            //}

            ui->update();
            // UI and console own keyboard/mouse input while visible.
            sceneManager->Update(
                (input->isUIActive() || input->isConsoleActive()) ? nullptr : input);

            if (auto* editor = m_context->getEditor()) editor->update();

            auto& physics = Lindo::Components::Physics::PhysicsSystem::GetInstance();
            physics.Step();

            Lindo::Components::Audio::AudioSystem::getInstance().update();

            if (debugSystem) {
                debugSystem->update(
                    sceneManager,
                    nullptr
                );
            }

            renderer->render();
            m_window->swapBuffers();

            Lindo::FrameManager::EndFrame();
        }

        LOG_INFO("[Application] Main loop stopped; context cleanup is owned by EngineContext.");
    }

}