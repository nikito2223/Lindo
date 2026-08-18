#pragma once
#include <memory> 

namespace Lindo {
    class SceneManager;

    namespace Graphics {
        namespace UI {
            class UIManager;
        }
        class Shader;
        class Skybox;
        class DebugDraw;
        class ShadowManager;
    }

    namespace Debug {
        class DebugSystem;
    }
}

namespace Lindo {
    namespace Graphics {
        class Renderer {
        public:
            Renderer(Lindo::SceneManager* scene, Lindo::Graphics::UI::UIManager* ui, Lindo::Debug::DebugSystem* debugSystem);
            ~Renderer();

            void init();
            void render();
            void onResize(int width, int height);

        private:
            Lindo::SceneManager* m_sceneManager = nullptr;
            Lindo::Graphics::UI::UIManager* m_uiManager = nullptr;
            Lindo::Debug::DebugSystem* m_debugSystem = nullptr;

            std::unique_ptr<Lindo::Graphics::Shader> m_lightingShader;
            std::unique_ptr<Lindo::Graphics::Skybox> m_skybox;
            std::unique_ptr<Lindo::Graphics::DebugDraw> m_debugDraw;
            std::unique_ptr<Lindo::Graphics::ShadowManager> m_shadowManager;

            bool m_showLightIcons = true;
            float m_lightIconRadius = 0.3f;
        };
    }
}