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
        class DebugOverlay;
    }
}

namespace Lindo {
    namespace Graphics {
        class Renderer {
        public:
            Renderer(Lindo::SceneManager* scene, Lindo::Graphics::UI::UIManager* ui, Lindo::Debug::DebugOverlay* debug);
            ~Renderer();

            void init();
            void render(float deltaTime);
            void onResize(int width, int height);

        private:
            Lindo::SceneManager* m_sceneManager;
            Lindo::Graphics::UI::UIManager* m_uiManager;
            Lindo::Debug::DebugOverlay* m_debugOverlay;

            std::unique_ptr<Lindo::Graphics::Shader> m_lightingShader;
            std::unique_ptr<Lindo::Graphics::Shader> m_skyboxShader;
            std::unique_ptr<Lindo::Graphics::Skybox> m_skybox;
            std::unique_ptr<Lindo::Graphics::DebugDraw> m_debugDraw;
            std::unique_ptr<Lindo::Graphics::ShadowManager> m_shadowManager;

            bool m_showLightIcons = true;
            float m_lightIconRadius = 0.3f;
        };
    }
}
