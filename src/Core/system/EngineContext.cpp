#include "EngineContext.h"
#include "Debug/DebugLogger.h"
#include "Core/RenderCommand.h"
#include "../Types/Settings.h"
#include <Component/Audio/AudioSystem.h>

namespace Lindo {

    bool EngineContext::init(Lindo::Window* window) {
        LOG_INFO("[EngineContext] Starting subsystem initialization...");

        auto& settings = Settings::getInstance();

        // 1. Создание подсистем
        m_input = std::make_unique<Lindo::Input::Input>();
        m_uiManager = std::make_unique<Lindo::Graphics::UI::UIManager>();
        m_debugSystem = std::make_unique<Lindo::Debug::DebugSystem>();
        m_sceneManager = &Lindo::SceneManager::getInstance();

        // 2. Рендер API и графические настройки
        Lindo::Graphics::IRenderAPI::SetAPI(Lindo::Graphics::GraphicsAPI::DirectX11);
        Lindo::Graphics::RenderCommand::Init();

        if (settings.msaaSamples > 0) {
            Lindo::Graphics::RenderCommand::SetMultisampling(true);
        }

        // 3. UI и окно
        m_uiManager->init(window->getWidth(), window->getHeight());
        window->setCallbacks(m_input.get(), m_uiManager.get());
        m_input->setConsole(m_uiManager->getConsole());

        // 4. Аудио
        auto& audio = Lindo::Components::Audio::AudioSystem::getInstance();
        if (!audio.init()) {
            LOG_ERROR("[EngineContext] Failed to initialize audio system!");
        }
        else {
            audio.setMasterVolume(settings.masterVolume);
        }
        // 5. Рендерер
        m_renderer = std::make_unique<Lindo::Graphics::Renderer>(
            m_sceneManager, m_uiManager.get(), m_debugSystem.get()
        );
        m_renderer->init();

        // 6. Сцены и отладка
        m_sceneManager->Init();
        m_sceneManager->LoadScene("Game");

        if (m_debugSystem && m_uiManager) {
            m_debugSystem->init(m_uiManager->getFont());
        }

        LOG_INFO("[EngineContext] All subsystems ready!");
        return true;
    }

    void EngineContext::cleanup() {
        LOG_INFO("[EngineContext] Cleaning up subsystems...");
        m_renderer.reset();
        m_debugSystem.reset();
        m_uiManager.reset();
        m_input.reset();
        Lindo::SceneManager::getInstance().Cleanup();
    }
}