#pragma once
#include <memory>
#include <string>
#include "Frustum.h"

namespace Lindo {
    struct Settings;
    class SceneManager;
    namespace World { class Scene; }
    namespace Components { namespace Rendering { class Camera; } }

    namespace Graphics {
        namespace UI {
            class UIManager;
        }
        class Shader;
        class Skybox;
        class DebugDraw;
        class ShadowManager;
        class Framebuffer;
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
            void applySettings(const Lindo::Settings& settings);
            bool isInitialized() const { return m_initialized; }
            Framebuffer* getFramebuffer() const { return m_fbo.get(); }

            bool setSkybox(const std::string& hdrRelativePath, int faceResolution = 1024);

            Skybox* getSkybox() const { return m_skybox.get(); }

        private:
            void renderScene(Lindo::World::Scene& scene,
                Lindo::Graphics::Shader& shader,
                Lindo::Components::Rendering::Camera* camera);

            Lindo::SceneManager* m_sceneManager = nullptr;
            Lindo::Graphics::UI::UIManager* m_uiManager = nullptr;
            Lindo::Debug::DebugSystem* m_debugSystem = nullptr;

            std::unique_ptr<Lindo::Graphics::Shader> m_lightingShader;
            std::unique_ptr<Lindo::Graphics::Skybox> m_skybox;
            std::unique_ptr<Lindo::Graphics::ShadowManager> m_shadowManager;
            std::unique_ptr<Lindo::Graphics::Framebuffer> m_fbo;

            int m_viewportWidth = 1280;  // ������� �� ���������
            int m_viewportHeight = 720;

            bool m_showLightIcons = true;
            float m_lightIconRadius = 0.3f;
            bool m_initialized = false;
            Frustum m_frustum;
        };
    }
}