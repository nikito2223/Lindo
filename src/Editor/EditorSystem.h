#pragma once

#include <memory>
#include <vector>
#include <string>

#include <glm/glm.hpp>

namespace Lindo {
    class SceneManager;
    namespace Input { class Input; }
    namespace World { class Scene; class GameObject; }
    namespace Graphics {
        class Frustum;
        namespace UI {
            class UIManager;
            class UIPanel;
            class UILabel;
        }
    }
    namespace Components { namespace Rendering { class Camera; } }
}

namespace Lindo::Editor {

    class EditorSystem {
    public:
        EditorSystem() = default;

        void initialize(Lindo::Graphics::UI::UIManager* ui,
            Lindo::SceneManager* scenes,
            Lindo::Input::Input* input);
        void toggle();
        void setEnabled(bool enabled);
        bool isEnabled() const { return m_enabled; }

        void update();
        void refreshInspector();

    private:
        struct VisibilityStats {
            int renderable = 0;
            int visible = 0;
            int culled = 0;
        };

        VisibilityStats collectVisibility(const Lindo::World::Scene* scene,
            const Lindo::Components::Rendering::Camera* camera) const;
        void setLabel(const std::string& id, const std::string& text);

        Lindo::Graphics::UI::UIManager* m_ui = nullptr;
        Lindo::SceneManager* m_sceneManager = nullptr;
        Lindo::Input::Input* m_input = nullptr;
        bool m_enabled = false;
        bool m_showBounds = true;
    };
}
