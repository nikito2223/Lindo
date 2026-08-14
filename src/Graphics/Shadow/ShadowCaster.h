#pragma once

#include "Graphics/Shadow/ShadowFramebuffer.h"
#include "Graphics/Shadow/ShadowQuality.h"
#include "Graphics/core/Shader.h"
#include <glm/glm.hpp>
#include <memory>
#include <vector>

// Forward declarations.
namespace Lindo { namespace World { class GameObject; } }
namespace Lindo { namespace Components { namespace Physics { class MeshRenderer; } } }

namespace Lindo {
namespace Graphics {

//==============================================================================
// ICascadeProvider - abstraction over an entity that can describe the
// directional-light cascades for the current frame. The ShadowManager
// implements this from the active directional light + camera.
//==============================================================================
struct CascadeData {
    glm::mat4 lightSpaceMatrix; // proj * view for this cascade
    float     splitDepth;       // far view-depth (ndc z) for the cascade
};

//==============================================================================
// ShadowCaster - base class for an object that renders geometry into a
// ShadowFramebuffer. Concrete casters own a framebuffer and the depth shader
// used to populate it.
//
// The SceneRenderer is a callback (implemented by ShadowManager) that draws
// every MeshRenderer into the currently-bound FBO using the given shader.
// This keeps the caster decoupled from the scene graph details.
//==============================================================================
class ShadowCaster {
public:
    using RenderSceneFn = void (*)(const std::vector<Lindo::World::GameObject*>& objects,
                                   Shader& depthShader);

    explicit ShadowCaster(RenderSceneFn renderScene);
    virtual ~ShadowCaster() = default;

    ShadowCaster(const ShadowCaster&) = delete;
    ShadowCaster& operator=(const ShadowCaster&) = delete;

    virtual void update(const ShadowQualitySettings& quality) = 0;
    virtual void render(const std::vector<Lindo::World::GameObject*>& shadowCasters) = 0;

unsigned int textureID() const { return m_target ? m_target->textureID() : 0; }
    bool valid() const { return m_target && m_target->valid(); }
    void bindForReading(unsigned int textureUnit) const {
        if (m_target) m_target->bindForReading(textureUnit);
    }

protected:
    RenderSceneFn m_renderScene = nullptr;
    std::unique_ptr<ShadowFramebuffer> m_target;
};

//==============================================================================
// DirectionalShadowCaster - Cascaded Shadow Maps (CSM).
//
// Renders the directional light into a GL_TEXTURE_2D_ARRAY, one layer per
// cascade. Cascade splits are computed on the CPU from the camera frustum;
// each cascade gets a tightly-fitted orthographic projection.
//==============================================================================
class DirectionalShadowCaster final : public ShadowCaster {
public:
    explicit DirectionalShadowCaster(RenderSceneFn renderScene);

    void setCascadeCount(int count);
    void setLight(const glm::vec3& lightDir);
    void setLightPosition(const glm::vec3& pos);
    void setCamera(const glm::mat4& viewProj, float nearPlane, float farPlane);

    void update(const ShadowQualitySettings& quality) override;
    void render(const std::vector<Lindo::World::GameObject*>& shadowCasters) override;

    int cascadeCount() const { return m_cascadeCount; }
    const std::vector<glm::mat4>& lightSpaceMatrices() const { return m_lightSpaceMatrices; }
    const std::vector<float>& splitDepths() const { return m_splitDepths; }

private:
    void computeCascades(float zNear, float zFar);
    void computeFrustumCorners(glm::vec3 corners[8], float zNear, float zFar) const;

    glm::vec3        m_lightDir     = glm::vec3(0.0f, -1.0f, 0.0f);
    glm::vec3        m_lightPos     = glm::vec3(0.0f, 20.0f, 10.0f);
    glm::mat4        m_viewProj     = glm::mat4(1.0f);
    float            m_near         = 0.1f;
    float            m_far          = 1000.0f;

    int              m_cascadeCount = 4;
    std::vector<glm::mat4> m_lightSpaceMatrices;
    std::vector<float>     m_splitDepths;
};

//==============================================================================
// PointShadowCaster - omnidirectional cubemap shadows for point lights.
// Renders all six faces of a cube into a GL_TEXTURE_CUBE_MAP depth target.
//==============================================================================
class PointShadowCaster final : public ShadowCaster {
public:
    explicit PointShadowCaster(RenderSceneFn renderScene);

    void setLight(const glm::vec3& pos, float farPlane);
    void update(const ShadowQualitySettings& quality) override;
    void render(const std::vector<Lindo::World::GameObject*>& shadowCasters) override;

private:
    glm::vec3 m_lightPos = glm::vec3(0.0f);
    float     m_farPlane = 25.0f;
};

//==============================================================================
// SpotShadowCaster - single 2D shadow map for spot lights.
//==============================================================================
class SpotShadowCaster final : public ShadowCaster {
public:
    explicit SpotShadowCaster(RenderSceneFn renderScene);

    void setLight(const glm::vec3& pos, const glm::vec3& direction,
                  float farPlane, float fullFovDegrees);
    void update(const ShadowQualitySettings& quality) override;
    void render(const std::vector<Lindo::World::GameObject*>& shadowCasters) override;

    const glm::mat4& lightSpaceMatrix() const { return m_lightSpaceMatrix; }

private:
    glm::vec3 m_lightPos = glm::vec3(0.0f);
    glm::vec3 m_lightDir = glm::vec3(0.0f, -1.0f, 0.0f);
    float     m_farPlane = 25.0f;
    float     m_fov = 45.0f;
    glm::mat4 m_lightSpaceMatrix = glm::mat4(1.0f);
};

} // namespace Graphics
} // namespace Lindo
