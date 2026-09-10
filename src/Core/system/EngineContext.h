#pragma once
#include <memory>
#include "Window/Window.h"
#include "../Input.h"
#include "../SceneManager.h"
#include "Graphics/ui/UIManager.h"
#include "Graphics/core/Renderer.h"
#include "Debug/DebugSystem.h"
#include "Editor/EditorSystem.h"

namespace Lindo {

    class EngineContext {
    public:
        EngineContext() = default;
        ~EngineContext();

        bool init(Lindo::Window* window);
        void cleanup();

        Lindo::Input::Input* getInput() const { return m_input.get(); }
        Lindo::Graphics::UI::UIManager* getUI() const { return m_uiManager.get(); }
        Lindo::Graphics::Renderer* getRenderer() const { return m_renderer.get(); }
        Lindo::Debug::DebugSystem* getDebugSystem() const { return m_debugSystem.get(); }
        Lindo::Editor::EditorSystem* getEditor() const { return m_editor.get(); }
        Lindo::SceneManager* getSceneManager() const { return m_sceneManager; }
        bool isInitialized() const { return m_initialized; }

    private:
        std::unique_ptr<Lindo::Input::Input> m_input;
        std::unique_ptr<Lindo::Graphics::UI::UIManager> m_uiManager;
        std::unique_ptr<Lindo::Debug::DebugSystem> m_debugSystem;
        std::unique_ptr<Lindo::Editor::EditorSystem> m_editor;
        std::unique_ptr<Lindo::Graphics::Renderer> m_renderer;
        Lindo::SceneManager* m_sceneManager = nullptr;
        bool m_initialized = false;
    };

}