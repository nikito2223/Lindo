#include "ShadowCaster.h"

#include "Component/Physhcs/MeshRenderer.h"
#include "Component/GameObject/GameObject.h"
#include "Debug/DebugLogger.h"

#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <array>
#include <cfloat>

namespace Lindo {
namespace Graphics {

//==============================================================================
// ShadowCaster (base)
//==============================================================================
ShadowCaster::ShadowCaster(RenderSceneFn renderScene)
    : m_renderScene(renderScene) {
}

//==============================================================================
// DirectionalShadowCaster - Cascaded Shadow Maps (CSM)
//==============================================================================
namespace {
// Practical split scheme: blend uniform and logarithmic depth splits.
std::vector<float> computeSplitDepths(float zNear, float zFar, int count) {
    std::vector<float> splits(count + 1, 0.0f);
    if (count <= 0) return splits;

    constexpr float lambda = 0.5f;
    splits[0] = zNear;
    splits[count] = zFar;

    for (int i = 1; i < count; ++i) {
        const float f = static_cast<float>(i) / static_cast<float>(count);
        const float logSplit = zNear * std::pow(zFar / zNear, f);
        const float uniformSplit = zNear + (zFar - zNear) * f;
        splits[i] = glm::mix(uniformSplit, logSplit, lambda);
    }
    return splits;
}
} // namespace

DirectionalShadowCaster::DirectionalShadowCaster(RenderSceneFn renderScene)
    : ShadowCaster(renderScene) {
}

void DirectionalShadowCaster::setCascadeCount(int count) {
    m_cascadeCount = glm::clamp(count, 1, 4);
}

void DirectionalShadowCaster::setLight(const glm::vec3& lightDir) {
    m_lightDir = lightDir;
}

void DirectionalShadowCaster::setLightPosition(const glm::vec3& pos) {
    m_lightPos = pos;
}

void DirectionalShadowCaster::setCamera(const glm::mat4& viewProj, float nearPlane, float farPlane) {
    m_viewProj = viewProj;
    m_near = nearPlane;
    m_far = farPlane;
}

void DirectionalShadowCaster::computeFrustumCorners(glm::vec3 corners[8], float zNear, float zFar) const {
    const glm::mat4 invViewProj = glm::inverse(m_viewProj);

    // Full frustum corners (near..far) for the camera's current viewProj.
    glm::vec3 fullCorners[8];
    for (int c = 0; c < 8; ++c) {
        const glm::vec4 ndc(
            (c & 1) ? 1.0f : -1.0f,
            (c & 2) ? 1.0f : -1.0f,
            (c & 4) ? 1.0f : -1.0f,
            1.0f);
        glm::vec4 world = invViewProj * ndc;
        world /= world.w;
        fullCorners[c] = glm::vec3(world);
    }

    // Interpolate each corner ray to the requested [zNear, zFar] slice.
    for (int c = 0; c < 8; ++c) {
        const float isFar = (c & 4) ? 1.0f : 0.0f;
        const float targetDepth = isFar ? zFar : zNear;
        const glm::vec3 nearPos = fullCorners[c & ~4];
        const glm::vec3 farPos  = fullCorners[c | 4];
        const float t = (m_far - m_near) > 0.0f
                            ? (targetDepth - m_near) / (m_far - m_near)
                            : 0.0f;
        corners[c] = glm::mix(nearPos, farPos, t);
    }
}

void DirectionalShadowCaster::computeCascades(float zNear, float zFar) {
    if (m_cascadeCount <= 0) {
        m_lightSpaceMatrices.clear();
        m_splitDepths.clear();
        return;
    }

    const float actualNear = glm::max(zNear, 0.01f);
    const float actualFar  = glm::max(zFar, actualNear + 0.1f);

    const std::vector<float> splits = computeSplitDepths(actualNear, actualFar, m_cascadeCount);

    // Light-space basis.
    const glm::vec3 lightDir = glm::normalize(m_lightDir);
    const glm::vec3 up = (std::fabs(lightDir.y) < 0.999f)
                             ? glm::vec3(0.0f, 1.0f, 0.0f)
                             : glm::vec3(1.0f, 0.0f, 0.0f);
    const glm::vec3 right = glm::normalize(glm::cross(up, lightDir));
    const glm::vec3 upv = glm::normalize(glm::cross(lightDir, right));

    m_lightSpaceMatrices.resize(m_cascadeCount);
    m_splitDepths.resize(m_cascadeCount);

    for (int i = 0; i < m_cascadeCount; ++i) {
        const float nearZ = splits[i];
        const float farZ  = splits[i + 1];

        glm::vec3 corners[8];
        computeFrustumCorners(corners, nearZ, farZ);

glm::vec3 center = glm::vec3(0.0f);
        for (int c = 0; c < 8; ++c) center += corners[c];
        center /= 8.0f;

        // Bound the frustum slice with a sphere so we can place the light eye
        // far enough that the shadow rays are near-parallel (directional).
        float radius = 0.0f;
        for (int c = 0; c < 8; ++c) {
            radius = glm::max(radius, glm::length(corners[c] - center));
        }
        const float lightDist = radius * 2.0f + 50.0f;
        // Position the light ALONG the light direction so the shadow map is
        // always aligned with the actual light direction, regardless of the
        // camera orientation or the light object's world position.
        const glm::vec3 lightEye = center - lightDir * lightDist;
        const glm::mat4 lightView = glm::lookAt(lightEye, center, upv);

        glm::vec3 minAABB(FLT_MAX);
        glm::vec3 maxAABB(-FLT_MAX);
        for (int c = 0; c < 8; ++c) {
            const glm::vec4 lsp = lightView * glm::vec4(corners[c], 1.0f);
            minAABB = glm::min(minAABB, glm::vec3(lsp));
            maxAABB = glm::max(maxAABB, glm::vec3(lsp));
        }

        // Extend the near plane outward so geometry behind the frustum slice
        // can still cast into it.
        const float margin = glm::max((maxAABB.z - minAABB.z) * 0.5f, 1.0f);
        minAABB.z -= margin;

        const glm::mat4 lightProj = glm::ortho(minAABB.x, maxAABB.x,
                                                minAABB.y, maxAABB.y,
                                                minAABB.z, maxAABB.z);

        m_lightSpaceMatrices[i] = lightProj * lightView;
        m_splitDepths[i] = farZ;
    }
}

void DirectionalShadowCaster::update(const ShadowQualitySettings& quality) {
    const int count = glm::clamp(quality.cascadeCount, 1, 4);
    if (m_target && m_target->valid() &&
        m_target->size() == quality.shadowMapSize &&
        count == m_target->layers()) {
        m_cascadeCount = count;
        return;
    }

    m_cascadeCount = count;
    m_target = std::make_unique<ShadowFramebuffer>();
    if (!m_target->init(ShadowFramebuffer::MountType::Array2D,
                        quality.shadowMapSize, count)) {
        m_target.reset();
    }
}

void DirectionalShadowCaster::render(const std::vector<Lindo::World::GameObject*>& shadowCasters) {
    if (!m_target || !m_target->valid() || !m_renderScene) return;

    static Shader* s_depthShader = nullptr;
    if (!s_depthShader) {
        static Shader depthShader(
            std::string("res/shaders/Shadow/csm_depth.vs").c_str(),
            std::string("res/shaders/Shadow/csm_depth.fs").c_str());
        s_depthShader = &depthShader;
        LOG_INFO("DirectionalShadowCaster: CSM depth shader loaded (ID=" +
                 std::to_string(depthShader.getID()) + ").");
    }

    computeCascades(m_near, m_far);

    s_depthShader->use();
    s_depthShader->setMat4("u_lightSpaceMatrix", m_lightSpaceMatrices[0]);

    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LESS);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_FRONT); // front-face culling avoids peter-panning
    glDisable(GL_BLEND);

for (int i = 0; i < m_cascadeCount; ++i) {
        m_target->beginRender(i);
        s_depthShader->setMat4("u_lightSpaceMatrix", m_lightSpaceMatrices[i]);
        m_renderScene(shadowCasters, *s_depthShader);
    }

    glCullFace(GL_BACK);
}

//==============================================================================
// PointShadowCaster - omnidirectional cubemap shadows
//==============================================================================
PointShadowCaster::PointShadowCaster(RenderSceneFn renderScene)
    : ShadowCaster(renderScene) {
}

void PointShadowCaster::setLight(const glm::vec3& pos, float farPlane) {
    m_lightPos = pos;
    m_farPlane = farPlane;
}

void PointShadowCaster::update(const ShadowQualitySettings& quality) {
    const unsigned int size = static_cast<unsigned int>(quality.pointShadowSize);
    if (m_target && m_target->valid() && m_target->size() == size) return;

    m_target = std::make_unique<ShadowFramebuffer>();
    if (!m_target->init(ShadowFramebuffer::MountType::Cube, size, 6)) {
        m_target.reset();
    }
}

void PointShadowCaster::render(const std::vector<Lindo::World::GameObject*>& shadowCasters) {
    if (!m_target || !m_target->valid() || !m_renderScene) return;

    static Lindo::Graphics::Shader* s_depthShader = nullptr;
    if (!s_depthShader) {
        static Lindo::Graphics::Shader depthShader(
            std::string("res/shaders/Shadow/point_depth.vs"),
            std::string("res/shaders/Shadow/point_depth.gs"),
            std::string("res/shaders/Shadow/point_depth.fs")
        );
        s_depthShader = &depthShader;
    }

    const float farPlane = m_farPlane;
    const glm::vec3 pos = m_lightPos;

    const glm::mat4 shadowProj = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, farPlane);
    const std::array<glm::mat4, 6> transforms = {
        shadowProj * glm::lookAt(pos, pos + glm::vec3( 1.0f, 0.0f, 0.0f), glm::vec3( 0.0f, -1.0f, 0.0f)),
        shadowProj * glm::lookAt(pos, pos + glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3( 0.0f, -1.0f, 0.0f)),
        shadowProj * glm::lookAt(pos, pos + glm::vec3( 0.0f, 1.0f, 0.0f), glm::vec3( 0.0f,  0.0f, 1.0f)),
        shadowProj * glm::lookAt(pos, pos + glm::vec3( 0.0f, -1.0f, 0.0f), glm::vec3( 0.0f,  0.0f, -1.0f)),
        shadowProj * glm::lookAt(pos, pos + glm::vec3( 0.0f, 0.0f, 1.0f), glm::vec3( 0.0f, -1.0f, 0.0f)),
        shadowProj * glm::lookAt(pos, pos + glm::vec3( 0.0f, 0.0f, -1.0f), glm::vec3( 0.0f, -1.0f, 0.0f))
    };

    s_depthShader->use();
    s_depthShader->setVec3("u_lightPos", pos);
    s_depthShader->setFloat("u_farPlane", farPlane);
    for (int f = 0; f < 6; ++f) {
        s_depthShader->setMat4("u_shadowMatrices[" + std::to_string(f) + "]", transforms[f]);
    }

    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LESS);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_FRONT);
    glDisable(GL_BLEND);

    // With the geometry shader writing gl_Layer for all 6 faces, a single
    // render pass fills the whole cubemap.
    m_target->beginRender(0);
    m_renderScene(shadowCasters, *s_depthShader);

    glCullFace(GL_BACK);
}

//==============================================================================
// SpotShadowCaster - single 2D shadow map for spot lights.
//==============================================================================
SpotShadowCaster::SpotShadowCaster(RenderSceneFn renderScene)
    : ShadowCaster(renderScene) {
}

void SpotShadowCaster::setLight(const glm::vec3& pos, const glm::vec3& direction,
                                float farPlane, float fullFovDegrees) {
    m_lightPos = pos;
    m_lightDir = glm::normalize(direction);
    m_farPlane = farPlane;
    m_fov = glm::clamp(fullFovDegrees, 1.0f, 179.0f);
}

void SpotShadowCaster::update(const ShadowQualitySettings& quality) {
    const unsigned int size = static_cast<unsigned int>(quality.spotShadowSize);
    if (m_target && m_target->valid() && m_target->size() == size) return;

    m_target = std::make_unique<ShadowFramebuffer>();
    if (!m_target->init(ShadowFramebuffer::MountType::Texture2D, size, 1)) {
        m_target.reset();
    }
}

void SpotShadowCaster::render(const std::vector<Lindo::World::GameObject*>& shadowCasters) {
    if (!m_target || !m_target->valid() || !m_renderScene) return;

    static Shader* s_depthShader = nullptr;
    if (!s_depthShader) {
        static Shader depthShader(
            std::string("res/shaders/Shadow/csm_depth.vs").c_str(),
            std::string("res/shaders/Shadow/csm_depth.fs").c_str());
        s_depthShader = &depthShader;
    }

    const glm::vec3 lightPos = m_lightPos;
    const glm::vec3 lightDir = glm::normalize(m_lightDir);
    const glm::vec3 up = (std::fabs(lightDir.y) < 0.999f)
                             ? glm::vec3(0.0f, 1.0f, 0.0f)
                             : glm::vec3(1.0f, 0.0f, 0.0f);
    const glm::mat4 lightView = glm::lookAt(lightPos, lightPos + lightDir, up);
    const glm::mat4 lightProj = glm::perspective(glm::radians(m_fov), 1.0f, 0.1f, m_farPlane);

    m_lightSpaceMatrix = lightProj * lightView;

    s_depthShader->use();
    s_depthShader->setMat4("u_lightSpaceMatrix", m_lightSpaceMatrix);
    s_depthShader->setMat4("u_model", glm::mat4(1.0f));

    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LESS);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_FRONT);
    glDisable(GL_BLEND);

    m_target->beginRender(0);
    m_renderScene(shadowCasters, *s_depthShader);
    glCullFace(GL_BACK);
}

} // namespace Graphics
} // namespace Lindo
