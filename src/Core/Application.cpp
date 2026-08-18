#include "Application.h"
#include "Window/Window.h"
#include <debug/Console.h>
#include <Physics/PhysicsSystem.h>
#include <Component/Audio/AudioSystem.h>
#include "Types/Settings.h"
#include "Core/OGL.h"
#include "Core/Time/Time.h"
#include "Core/FrameManager.h" // Подключаем менеджер кадров

#include "Debug/DebugSystem.h"
#include "Debug/DebugOverlay.h"
#include "Debug/ConsoleCommands.h"

#include <algorithm>
#include <thread>
#include <GLFW/glfw3.h>

#ifdef _WIN32
extern "C" {
    __declspec(dllexport) unsigned long NvOptimusEnablement = 0x00000001;
    __declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}
#endif

namespace Lindo {

    Application::Application() {
        Lindo::Core::DebugLogger::Init("engine_log.txt");
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

    Application::~Application() {
        LOG_INFO("==================================================");
        LOG_INFO(std::string(AppInfo::Name) + " shutting down...");
        LOG_INFO("==================================================");
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
        m_context->init(m_window.get());
        Lindo::ConsoleCommands::RegisterAll(m_context.get(), m_window.get());
        logGPUInfo();

        LOG_INFO("[Application] Entering main loop...");

        DisplaySettings& displaySettings = DisplaySettings::getInstance();

        // Инициализируем FrameManager значением из настроек экрана
        Lindo::FrameManager::Init(displaySettings.targetFPS);

        auto* input = m_context->getInput();
        auto* ui = m_context->getUI();
        auto* renderer = m_context->getRenderer();
        auto* debugSystem = m_context->getDebugSystem();
        auto* sceneManager = m_context->getSceneManager();

        while (!m_window->shouldClose()) {
            // 1. Фиксируем старт кадра для точной синхронизации
            Lindo::FrameManager::BeginFrame();

            // 2. Обновляем глобальное время игрового движка
            Lindo::Time::Update();

            m_window->pollEvents();
            input->update(m_window.get());

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

            if (input->consumeEscape()) {
                input->setUIActive(!input->isUIActive());
            }

            // --- UPDATE ---
            ui->update();
            sceneManager->Update(input);

            auto& physics = Lindo::Components::Physics::PhysicsSystem::GetInstance();
            physics.Step();

            Lindo::Components::Audio::AudioSystem::getInstance().update();

            // Передаем точный средний FPS и фреймтайм в оверлей отладки
            debugSystem->getOverlay()->updateStats(
                static_cast<int>(Lindo::FrameManager::GetFPS()),
                debugSystem->getOverlay()->isVisible()
            );

            // --- RENDER ---
            renderer->render();
            m_window->swapBuffers();

            // 3. Завершаем кадр: ограничиваем FPS и рассчитываем метрики нагрузки
            Lindo::FrameManager::EndFrame();
        }

        m_context->cleanup();
    }

}