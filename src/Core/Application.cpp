#include "Application.h"
#include "Window/Window.h"
#include "Input.h"
#include "SceneManager.h"
#include "Graphics/ui/UIManager.h"
#include "debug/DebugOverlay.h"
#include "Graphics/core/Renderer.h"
#include "core/Globals.h"
#include <Physics/PhysicsSystem.h>
#include <Component/Audio/AudioSystem.h>

#ifdef _WIN32
extern "C" {
    __declspec(dllexport) unsigned long NvOptimusEnablement = 0x00000001;
    __declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}
#endif

namespace Lindo {

    Application::Application() {
        Lindo::Core::DebugLogger::Init("engine_log.txt");

        // Выводим красивую шапку при запуске
        logAppHeader();

        // Window создаёт OpenGL контекст и задает заголовок с версией
        m_window = std::make_unique<Lindo::Window>(
            Globals::screenWidth, 
            Globals::screenHeight, 
            AppInfo::GetFormattedTitle().c_str()
        );

        m_input = std::make_unique<Lindo::Input::Input>();
        m_sceneManager = &Lindo::SceneManager::getInstance();
        m_uiManager = std::make_unique<Lindo::Graphics::UI::UIManager>();
        m_debugOverlay = std::make_unique<Lindo::Debug::DebugOverlay>();

        m_renderer = std::make_unique<Lindo::Graphics::Renderer>(
            m_sceneManager, m_uiManager.get(), m_debugOverlay.get()
        );
    }

    Application::~Application() {
        LOG_INFO("==================================================");
        LOG_INFO(std::string(AppInfo::Name) + " shutting down...");
        LOG_INFO("==================================================");
        Lindo::Core::DebugLogger::Close();
    }

    void Application::logAppHeader() {
        LOG_INFO("==================================================");
        LOG_INFO("  _     _           _          ");
        LOG_INFO(" | |   (_)_ __   __| | ___     ");
        LOG_INFO(" | |   | | '_ \\ / _` |/ _ \\  ");
        LOG_INFO(" | |___| | | | | (_| | (_) |   ");
        LOG_INFO(" |_____|_|_| |_|\\__,_|\\___/  ");
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

    void Application::run() {
        LOG_INFO("Initializing window callbacks...");
        m_window->setCallbacks(m_input.get(), m_uiManager.get());
    
        // Audio
        if (!Lindo::Components::Audio::AudioSystem::getInstance().init()) {
            LOG_ERROR("Failed to initialize audio system");
        }
    
        // OpenGL state
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
        // MSAA должен быть включён в контексте GLFW,
        // а не обязательно здесь.
        glEnable(GL_MULTISAMPLE);
    
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    
        LOG_INFO("Initializing subsystems...");
    
        m_uiManager->init();
        m_sceneManager->Init();
        m_sceneManager->LoadScene("Game");
    
        m_debugOverlay->init(m_uiManager->getFont());
        m_renderer->init();
    
        LOG_INFO("Entering main loop...");
    
        double lastTime = glfwGetTime();
    
        float fpsTimer = 0.0f;
        int frameCount = 0;
    
        while (!m_window->shouldClose()) {
        
            // -----------------------------------------
            // TIME
            // -----------------------------------------
        
            const double currentTime = glfwGetTime();
        
            float deltaTime =
                static_cast<float>(currentTime - lastTime);
        
            lastTime = currentTime;
        
            // Защита от огромного dt после сворачивания окна
            deltaTime = std::min(deltaTime, 0.05f);
        
        
            // -----------------------------------------
            // EVENTS
            // -----------------------------------------
        
            m_window->pollEvents();
        
        
            // -----------------------------------------
            // INPUT
            // -----------------------------------------
        
            m_input->update(m_window.get());
        
            if (m_input->consumeF3()) {
                const bool debugMode =
                    m_debugOverlay->toggle();
            
                LOG_INFO(
                    std::string("Debug mode: ") +
                    (debugMode ? "ON" : "OFF")
                );
            }
        
            if (m_input->consumeF4()) {
                Globals::renderPhysicsDebug =
                    !Globals::renderPhysicsDebug;
            
                LOG_INFO(
                    std::string("Physics debug: ") +
                    (Globals::renderPhysicsDebug ? "ON" : "OFF")
                );
            }
        
            if (m_input->consumeF11()) {
                m_window->toggleFullscreen();
            }
        
            if (m_input->consumeEscape()) {
                m_input->setUIActive(
                    !m_input->isUIActive()
                );
            }
        
        
            // -----------------------------------------
            // UPDATE
            // -----------------------------------------
        
            m_sceneManager->Update(
                deltaTime,
                m_input.get()
            );
        
            auto& physics =
                Lindo::Components::Physics::PhysicsSystem::GetInstance();
        
            physics.Step(deltaTime);
        
            auto& audio =
                Lindo::Components::Audio::AudioSystem::getInstance();
        
            audio.update(deltaTime);
        
        
            // -----------------------------------------
            // FPS
            // -----------------------------------------
        
            fpsTimer += deltaTime;
            ++frameCount;
        
            if (fpsTimer >= 1.0f) {
            
                m_debugOverlay->updateStats(
                    fpsTimer,
                    frameCount,
                    m_debugOverlay->isVisible()
                );
            
                fpsTimer -= 1.0f;
                frameCount = 0;
            }
        
        
            // -----------------------------------------
            // RENDER
            // -----------------------------------------
        
            m_renderer->render(deltaTime);
        
            m_window->swapBuffers();
        }
    
        m_sceneManager->Cleanup();
    }
}