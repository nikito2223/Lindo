#pragma once

#include "Graphics/Shadow/ShadowQuality.h"
#include "Graphics/core/Shader.h"
#include <glm/glm.hpp>
#include <memory>
#include <vector>

namespace Lindo { namespace World { class Scene; class GameObject; } }
namespace Lindo { namespace Components { namespace Light { class DirectionalLight; class PointLight; } } }
namespace Lindo { namespace Components { namespace Rendering { class Camera; } } }

namespace Lindo {
namespace Graphics {

class DirectionalShadowCaster;
class PointShadowCaster;
class SpotShadowCaster;

//==============================================================================
// ShadowManager - top-level orchestrator of all shadow rendering.
//
// Responsibilities:
//   * Own the directional (CSM) caster and a pool of point casters.
//   * Discover shadow-casting geometry from the active scene.
//   * Drive the shadow render passes each frame.
//   * Bind the produced shadow textures + uniforms for the forward pass.
//   * Expose a runtime quality API (setQuality).
//
// The manager is decoupled from the renderer: it only needs a Scene (to gather
// lights + shadow casters) and a Camera (for cascade splits).
//==============================================================================
class ShadowManager {
public:
    ShadowManager();
    ~ShadowManager();

    ShadowManager(const ShadowManager&) = delete;
    ShadowManager& operator=(const ShadowManager&) = delete;

    // Runtime quality switching. Reallocates shadow targets as needed.
    void setQuality(ShadowQuality quality);
    ShadowQuality quality() const { return m_quality; }
    const ShadowQualitySettings& settings() const { return m_settings; }

    // Build/refresh shadow resources for the active scene.
    void initialize(Lindo::World::Scene* scene);

    // Render all shadow passes for the current frame. 'viewProj' is the
    // camera's projection * view; near/far are the camera clip planes.
    void renderShadows(Lindo::World::Scene* scene,
                       const glm::mat4& viewProj, float nearPlane, float farPlane);

    // Bind the CSM texture + point shadow textures and set their uniforms on
    // the given forward shader. Call after renderShadows().
    void bindShadowTextures(Shader& forwardShader, const Lindo::World::Scene* scene) const;

    // Optional: bind individual texture units for manual binding.
    unsigned int directionalShadowTexture() const;
    bool directionalShadowsActive() const { return m_directionalActive; }
    int cascadeCount() const;
    int pointShadowIndexFor(const Components::Light::PointLight* light) const;

private:
    static void renderSceneCallback(const std::vector<Lindo::World::GameObject*>& objects,
                                    Shader& depthShader);

    void discoverCasters(Lindo::World::Scene* scene);
    void updatePointCasters(Lindo::World::Scene* scene);

    ShadowQuality m_quality = ShadowQuality::Ultra;
    ShadowQualitySettings m_settings;

    std::unique_ptr<DirectionalShadowCaster> m_directionalCaster;
    std::vector<std::unique_ptr<PointShadowCaster>> m_pointCasters;

    // Cached per-frame data.
    std::vector<Lindo::World::GameObject*> m_shadowCasters;
    std::vector<const Components::Light::PointLight*> m_pointLightSources;
    std::vector<glm::vec3> m_pointLightPositions;
    std::vector<float> m_pointLightFarPlanes;
    std::vector<int> m_pointLightShadowIndices;
    std::vector<glm::mat4> m_lightSpaceMatrices;
    std::vector<float> m_splitDepths;

    std::unique_ptr<SpotShadowCaster> m_spotCaster;
    glm::mat4 m_spotLightSpaceMatrix = glm::mat4(1.0f);
    glm::vec3 m_spotLightPosition = glm::vec3(0.0f);
    glm::vec3 m_spotLightDirection = glm::vec3(0.0f, -1.0f, 0.0f);
    float m_spotLightFarPlane = 0.0f;
    bool m_spotActive = false;

    bool m_directionalActive = false;
    bool m_initialized = false;
    bool m_diagnosticsLogged = false;
};

} // namespace Graphics
} // namespace Lindo
