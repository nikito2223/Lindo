#include "EditorSystem.h"

#include "Core/Input.h"
#include "Core/SceneManager.h"
#include "Component/GameObject/GameObject.h"
#include "Component/Camera/Camera.h"
#include "Component/PlayerController/Player.h"
#include "Component/Physhcs/MeshRenderer.h"
#include "Graphics/core/Frustum.h"
#include "Graphics/core/Model.h"
#include "Graphics/ui/UIManager.h"
#include "Graphics/ui/UIWidget.h"
#include "Scripting/LuaApi.h"
#include "Scripting/LuaRuntime.h"
#include "Core/AssetManager.h"
#include "world/Scene.h"

#include <glm/gtc/matrix_transform.hpp>
#include <limits>
#include <sstream>

namespace {
    Lindo::Math::AABB worldBounds(const Lindo::World::GameObject& object,
        const Lindo::Graphics::Model& model) {
        const auto local = model.getAABB();
        const glm::mat4 world = object.getWorldMatrix();
        glm::vec3 min = glm::vec3(std::numeric_limits<float>::max());
        glm::vec3 max = glm::vec3(std::numeric_limits<float>::lowest());
        const glm::vec3 corners[] = {
            { local.min.x, local.min.y, local.min.z }, { local.max.x, local.min.y, local.min.z },
            { local.min.x, local.max.y, local.min.z }, { local.max.x, local.max.y, local.min.z },
            { local.min.x, local.min.y, local.max.z }, { local.max.x, local.min.y, local.max.z },
            { local.min.x, local.max.y, local.max.z }, { local.max.x, local.max.y, local.max.z }
        };
        for (const glm::vec3& corner : corners) {
            const glm::vec3 point = glm::vec3(world * glm::vec4(corner, 1.0f));
            min = glm::min(min, point);
            max = glm::max(max, point);
        }
        return { min, max };
    }

    Lindo::Components::Rendering::Camera* findCamera(const Lindo::World::Scene* scene) {
        if (!scene) return nullptr;
        for (auto* camera : scene->FindComponentsOfType<Lindo::Components::Rendering::Camera>()) {
            if (camera && camera->gameObject) return camera;
        }
        return nullptr;
    }
}

namespace Lindo::Editor {

    void EditorSystem::initialize(Lindo::Graphics::UI::UIManager* ui,
        Lindo::SceneManager* scenes,
        Lindo::Input::Input* input) {
        m_ui = ui;
        m_sceneManager = scenes;
        m_input = input;
        if (!m_ui || !m_ui->getRootPanel()) return;

        auto& runtime = Lindo::Scripting::LuaRuntime::Get();
        if (runtime.initialize()) {
            sol::environment environment(runtime.state(), sol::create, runtime.state().globals());
            runtime.executeFile(AssetManager::get().resolvePath("editor.lua", "scripts"), environment);
        }
    }

    void EditorSystem::toggle() {
        setEnabled(!m_enabled);
    }

    void EditorSystem::setEnabled(bool enabled) {
        m_enabled = enabled;
        Lindo::Scripting::LuaUI::SetVisible("editor.panel", enabled);
        if (m_input) m_input->setUIActive(enabled);
        refreshInspector();
    }

    void EditorSystem::update() {
        if (!m_enabled) return;
        refreshInspector();
    }

    EditorSystem::VisibilityStats EditorSystem::collectVisibility(
        const Lindo::World::Scene* scene,
        const Lindo::Components::Rendering::Camera* camera) const {
        VisibilityStats stats;
        if (!scene || !camera) return stats;

        Lindo::Graphics::Frustum frustum;
        frustum.update(camera->getProjectionMatrix() * camera->getViewMatrix());
        for (const auto& object : scene->GetGameObjects()) {
            if (!object || !object->isActive) continue;
            auto* renderer = object->getComponent<Lindo::Components::Physics::MeshRenderer>();
            if (!renderer || !renderer->IsEnabled() || !renderer->model) continue;
            ++stats.renderable;
            if (frustum.intersects(worldBounds(*object, *renderer->model))) ++stats.visible;
            else ++stats.culled;
        }
        return stats;
    }

    void EditorSystem::setLabel(const std::string& id, const std::string& text) {
        Lindo::Scripting::LuaUI::SetText(id, text);
    }

    void EditorSystem::refreshInspector() {
        if (!m_enabled || !m_sceneManager) return;
        auto* scene = m_sceneManager->GetCurrentScene();
        if (!scene) return;

        auto* player = scene->FindGameObject("Player");
        auto* camera = findCamera(scene);
        const auto stats = collectVisibility(scene, camera);

        setLabel("editor.scene", "Scene: " + scene->GetName());
        if (player) {
            const glm::vec3 position = player->getWorldPosition();
            setLabel("editor.player", "Player: " + player->getName());
            setLabel("editor.position", "Position: " + std::to_string(static_cast<int>(position.x)) + ", " +
                std::to_string(static_cast<int>(position.y)) + ", " +
                std::to_string(static_cast<int>(position.z)));
        }
        else {
            setLabel("editor.player", "Player: not found");
            setLabel("editor.position", "Position: -");
        }
        setLabel("editor.objects", "Objects: " + std::to_string(scene->GetGameObjects().size()));
        setLabel("editor.culling", "Culling: visible " + std::to_string(stats.visible) +
            " / culled " + std::to_string(stats.culled));
        setLabel("editor.bounds", "Bounds: " + std::string(m_showBounds ? "enabled" : "disabled"));

        std::ostringstream objects;
        size_t index = 0;
        for (const auto& object : scene->GetGameObjects()) {
            if (!object || index >= 8) continue;
            if (index > 0) objects << "  ";
            objects << object->getName();
            ++index;
        }
        setLabel("editor.object_list", objects.str().empty() ? "-" : objects.str());
    }
}
