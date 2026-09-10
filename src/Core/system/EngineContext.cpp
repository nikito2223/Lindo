#include "EngineContext.h"
#include "Debug/DebugLogger.h"
#include "Core/RenderCommand.h"
#include "Core/AssetManager.h"
#include "../Types/Settings.h"
#include <Component/Audio/AudioSystem.h>
#include "Scripting/LuaRuntime.h"
#include "Scripting/LuaApi.h"
#include <thread>
#include <chrono>

namespace Lindo {

    // Вспомогательный класс для точного замера времени инициализации этапа
    class ScopedTimer {
    public:
        ScopedTimer(const std::string& stepName)
            : m_name(stepName), m_start(std::chrono::high_resolution_clock::now()) {
            LOG_INFO("[EngineContext] START: " + m_name);
        }

        ~ScopedTimer() {
            auto end = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - m_start).count();
            LOG_INFO("[EngineContext] DONE: " + m_name + " [ took " + std::to_string(duration) + " ms ]");
        }

    private:
        std::string m_name;
        std::chrono::high_resolution_clock::time_point m_start;
    };

    EngineContext::~EngineContext() {
        cleanup();
    }

    bool EngineContext::init(Lindo::Window* window) {
        if (m_initialized) {
            LOG_WARN("[EngineContext] init() called more than once; keeping existing context.");
            return true;
        }
        if (!window || !window->getNativeWindow()) {
            LOG_ERROR("[EngineContext] Cannot initialize without a valid window.");
            return false;
        }

        auto totalStart = std::chrono::high_resolution_clock::now();
        LOG_INFO("==================================================");
        LOG_INFO("[EngineContext] Subsystem initialization profiling...");
        LOG_INFO("==================================================");

        auto& settings = Settings::getInstance();

        // 1. Создание подсистем
        {
            ScopedTimer timer("Core Systems Allocation");
            m_input = std::make_unique<Lindo::Input::Input>();
            m_uiManager = std::make_unique<Lindo::Graphics::UI::UIManager>();
            m_debugSystem = std::make_unique<Lindo::Debug::DebugSystem>();
            m_editor = std::make_unique<Lindo::Editor::EditorSystem>();
            m_sceneManager = &Lindo::SceneManager::getInstance();
        }

        // 2. Инициализация Render API
        {
            ScopedTimer timer("Render API Init");
            Lindo::Graphics::IRenderAPI::SetAPI(Lindo::Graphics::GraphicsAPI::OpenGL);
            Lindo::Graphics::RenderCommand::Init();
            if (settings.msaaSamples > 0) {
                Lindo::Graphics::RenderCommand::SetMultisampling(true);
            }
        }

        // 3. UIManager & Шрифты
        {
            ScopedTimer timer("UI Manager & Font Atlas Generation");
            m_uiManager->init(window->getWidth(), window->getHeight());
            window->setCallbacks(m_input.get(), m_uiManager.get());
            m_input->setConsole(m_uiManager->getConsole());

            m_input->setUIActive(true);
            m_input->setCharCallback([this](unsigned int codepoint) {
                if (m_uiManager) m_uiManager->onChar(codepoint);
            });
            m_input->setKeyCallback([this](int key, int action) {
                if (m_uiManager) m_uiManager->onKey(key, 0, action, 0);
            });
        }

        // 4. Аудио система (Фоновая инициализация)
        {
            ScopedTimer timer("Audio System Init (Async Launch)");

            // Запускаем открытие устройства OpenAL в фоновом потоке
            std::thread audioInitThread([]() {
                auto& audio = Lindo::Components::Audio::AudioSystem::getInstance();
                auto& settings = Settings::getInstance();

                if (settings.muteAudio) {
                    LOG_INFO("[AudioSystem] Muted by settings, skipping OpenAL init.");
                    return;
                }
            
                if (!audio.init()) {
                    LOG_WARN("[AudioSystem] Audio device initialization failed or non-existent.");
                } else {
                    audio.setMasterVolume(settings.masterVolume);
                }
            });
            audioInitThread.detach();
        
            // Слушатель настроек громкости
            Settings::getInstance().addOnChangedCallback([](const Settings& changedSettings) {
                auto& audio = Lindo::Components::Audio::AudioSystem::getInstance();
                if (audio.isInitialized()) {
                    audio.setMasterVolume(changedSettings.muteAudio ? 0.0f : changedSettings.masterVolume);
                }
            });
        }

        // 5. Рендерер
        {
            ScopedTimer timer("Renderer Init (Shaders & Buffers)");
            m_renderer = std::make_unique<Lindo::Graphics::Renderer>(
                m_sceneManager, m_uiManager.get(), m_debugSystem.get()
            );
            m_renderer->init();
        }

        // 6. Lua Runtime
        {
            ScopedTimer timer("Lua State & API Bindings Init");

            // Передаем 4 аргумента, включая m_renderer.get()
            Lindo::Scripting::SetLuaEngineContext(
                m_sceneManager, 
                m_input.get(), 
                m_uiManager.get(), 
                m_renderer.get()
            );

            if (!Lindo::Scripting::LuaRuntime::Get().initialize()) {
                LOG_ERROR("[EngineContext] Failed to initialize Lua Runtime!");
            }
        }

        // 7. Сцены & Bootstrap скрипт
        {
            ScopedTimer timer("Scene Manager & Bootstrap Execution");
            m_sceneManager->Init();

            sol::environment bootstrapEnvironment(
                Lindo::Scripting::LuaRuntime::Get().state(),
                sol::create,
                Lindo::Scripting::LuaRuntime::Get().state().globals());
            
            if (!Lindo::Scripting::LuaRuntime::Get().executeFile(
                Lindo::AssetManager::get().resolvePath("bootstrap.lua", "scripts"),
                bootstrapEnvironment)) {
                m_sceneManager->LoadScene("Game");
            }
        }

        // 8. Редактор и Отладка
        {
            ScopedTimer timer("Editor & Debug Systems Init");
            m_editor->initialize(m_uiManager.get(), m_sceneManager, m_input.get());

            if (m_debugSystem && m_uiManager) {
                m_debugSystem->init(m_uiManager->getFont());
            }
        }

        m_initialized = true;

        auto totalEnd = std::chrono::high_resolution_clock::now();
        auto totalDuration = std::chrono::duration_cast<std::chrono::milliseconds>(totalEnd - totalStart).count();

        LOG_INFO("==================================================");
        LOG_INFO("[EngineContext] ALL SUBSYSTEMS READY! Total Startup Time: " + std::to_string(totalDuration) + " ms");
        LOG_INFO("==================================================");

        return true;
    }

    void EngineContext::cleanup() {
        if (!m_initialized && !m_renderer && !m_uiManager && !m_input) return;

        const auto cleanupStart = std::chrono::steady_clock::now();
        LOG_INFO("[EngineContext] Cleaning up subsystems...");

        Lindo::SceneManager::getInstance().Cleanup();
        m_renderer.reset();

        Lindo::Components::Audio::AudioSystem::getInstance().shutdown();
        m_debugSystem.reset();
        m_editor.reset();
        m_uiManager.reset();
        m_input.reset();
        m_sceneManager = nullptr;
        m_initialized = false;

        const auto cleanupDuration = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - cleanupStart).count();
        LOG_INFO("[EngineContext] Cleaned up successfully in " +
            std::to_string(cleanupDuration) + " ms.");
    }
}