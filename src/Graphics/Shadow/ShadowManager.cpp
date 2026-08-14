#include "ShadowManager.h"

#include "Graphics/Shadow/ShadowCaster.h"
#include "Graphics/core/mesh.h"
#include "Component/Graphics/Light.h"
#include "Component/Physhcs/MeshRenderer.h"
#include "Component/Camera/Camera.h"
#include "Component/GameObject/GameObject.h"
#include "world/Scene.h"
#include "Debug/DebugLogger.h"

#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>

namespace Lindo {
namespace Graphics {

//==============================================================================
// ShadowManager
//==============================================================================
ShadowManager::ShadowManager() {
    m_settings = qualitySettingsOf(m_quality);
}

ShadowManager::~ShadowManager() = default;

void ShadowManager::setQuality(ShadowQuality quality) {
    if (quality == m_quality) return;
    m_quality = quality;
    m_settings = qualitySettingsOf(m_quality);

    // Cascade count / map size may have changed; reallocate targets lazily in
    // the next renderShadows() via update().
    if (m_directionalCaster) {
        m_directionalCaster->update(m_settings);
    }
    for (auto& pc : m_pointCasters) {
        if (pc) pc->update(m_settings);
    }
    if (m_spotCaster) {
        m_spotCaster->update(m_settings);
    }
}

void ShadowManager::initialize(Lindo::World::Scene* scene) {
    if (m_initialized) return;

    m_directionalCaster = std::make_unique<DirectionalShadowCaster>(&ShadowManager::renderSceneCallback);
    m_directionalCaster->update(m_settings);
    m_spotCaster = std::make_unique<SpotShadowCaster>(&ShadowManager::renderSceneCallback);
    m_spotCaster->update(m_settings);

    bool dirValid = m_directionalCaster && m_directionalCaster->valid();
    LOG_INFO("ShadowManager initialized. Directional caster valid: " + std::string(dirValid ? "true" : "false"));

    m_initialized = true;

    // Pre-cache shadow casters from the scene.
    discoverCasters(scene);
    updatePointCasters(scene);

    LOG_INFO("ShadowManager initialized (quality=" + std::to_string(static_cast<int>(m_quality)) +
             ", cascades=" + std::to_string(m_settings.cascadeCount) +
             ", mapSize=" + std::to_string(m_settings.shadowMapSize) + ")");
}

void ShadowManager::renderSceneCallback(const std::vector<Lindo::World::GameObject*>& objects,
                                        Shader& depthShader) {
    for (Lindo::World::GameObject* obj : objects) {
        if (!obj || !obj->castsShadows) continue;

        auto* meshRenderer = obj->getComponent<Lindo::Components::Physics::MeshRenderer>();
        if (!meshRenderer || !meshRenderer->IsEnabled()) continue;

        const glm::mat4 worldMatrix = obj->getWorldMatrix();
        depthShader.setMat4("u_model", worldMatrix);

        if (meshRenderer->mesh) {
            meshRenderer->mesh->Draw(depthShader);
        }
        else if (meshRenderer->model) {
            for (auto& mesh : meshRenderer->model->getMeshes()) {
                mesh.Draw(depthShader);
            }
        }
    }
}

void ShadowManager::discoverCasters(Lindo::World::Scene* scene) {
    m_shadowCasters.clear();
    if (!scene) return;

    const auto& objects = scene->GetGameObjects();
    for (const auto& obj : objects) {
        if (!obj || !obj->castsShadows) continue;
        auto* mr = obj->getComponent<Lindo::Components::Physics::MeshRenderer>();
        if (mr && (mr->mesh || mr->model)) {
            m_shadowCasters.push_back(obj.get());
        }
    }
    
    // 👇 ДОБАВИТЬ ЗДЕСЬ
    LOG_INFO("Shadow casters found: " + std::to_string(m_shadowCasters.size()));
}

void ShadowManager::updatePointCasters(Lindo::World::Scene* scene) {
    if (!scene) return;

    auto pointLights = scene->FindComponentsOfType<Lindo::Components::Light::PointLight>();
    const int maxPoint = m_settings.maxPointShadows;

    // Rebuild the point light snapshot.
    m_pointLightPositions.clear();
    m_pointLightFarPlanes.clear();
    m_pointLightSources.clear();
    for (auto* pl : pointLights) {
        if (!pl || !pl->enabled || !pl->castShadows) continue;
        if (static_cast<int>(m_pointLightPositions.size()) >= maxPoint) break;
        m_pointLightSources.push_back(pl);
        m_pointLightPositions.push_back(pl->getPosition());
        m_pointLightFarPlanes.push_back(pl->farPlane);
    }

    // Ensure we have enough point casters.
    const size_t needed = m_pointLightPositions.size();
    if (m_pointCasters.size() < needed) {
        m_pointCasters.resize(needed);
    }
    for (size_t i = 0; i < m_pointCasters.size(); ++i) {
        if (!m_pointCasters[i]) {
            m_pointCasters[i] = std::make_unique<PointShadowCaster>(&ShadowManager::renderSceneCallback);
        }
        if (m_pointCasters[i]) {
            m_pointCasters[i]->update(m_settings);
        }
    }
}

void ShadowManager::renderShadows(Lindo::World::Scene* scene,
                                  const glm::mat4& viewProj, float nearPlane, float farPlane) {
    if (!scene) return;
    if (!m_initialized) initialize(scene);

    discoverCasters(scene);
    updatePointCasters(scene);

    LOG_INFO("Rendering shadows for " + std::to_string(m_shadowCasters.size()) + " casters");

    if (m_settings.cascadeCount <= 0) {
        m_directionalActive = false;
    }

    // --- Directional (CSM) pass ---
    auto* dirLight = scene->FindComponentOfType<Lindo::Components::Light::DirectionalLight>();
    m_directionalActive = false;
    if (dirLight && dirLight->enabled && dirLight->castShadows && m_directionalCaster) {
        m_directionalCaster->setLight(dirLight->direction);
        m_directionalCaster->setLightPosition(dirLight->getPosition());
        m_directionalCaster->setCamera(viewProj, nearPlane, farPlane);
        m_directionalCaster->render(m_shadowCasters);

        m_lightSpaceMatrices = m_directionalCaster->lightSpaceMatrices();
        m_splitDepths = m_directionalCaster->splitDepths();
        m_directionalActive = m_directionalCaster->valid();
    }

    // --- Point light passes ---
    for (size_t i = 0; i < m_pointLightPositions.size(); ++i) {
        if (i >= m_pointCasters.size() || !m_pointCasters[i]) continue;
        m_pointCasters[i]->setLight(m_pointLightPositions[i], m_pointLightFarPlanes[i]);
        m_pointCasters[i]->render(m_shadowCasters);
    }

    // --- Spot light pass ---
    m_spotActive = false;
    auto* spotLight = scene->FindComponentOfType<Lindo::Components::Light::SpotLight>();
    if (spotLight && spotLight->enabled && spotLight->castShadows && m_spotCaster) {
        glm::vec3 lightDirection = spotLight->direction;
        if (spotLight->owner) {
            lightDirection = glm::normalize(glm::mat3(spotLight->owner->getWorldMatrix()) * spotLight->direction);
        }
        float coneAngle = glm::degrees(glm::acos(glm::clamp(spotLight->cutOff, -1.0f, 1.0f)));
        m_spotCaster->setLight(spotLight->getPosition(), lightDirection,
                               spotLight->farPlane, coneAngle);
        m_spotCaster->render(m_shadowCasters);
        m_spotLightSpaceMatrix = m_spotCaster->lightSpaceMatrix();
        m_spotLightPosition = spotLight->getPosition();
        m_spotLightDirection = lightDirection;
        m_spotLightFarPlane = spotLight->farPlane;
        m_spotActive = m_spotCaster->valid();
    }

    // Restore default framebuffer + viewport for the forward pass.
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void ShadowManager::bindShadowTextures(Shader& forwardShader, const Lindo::World::Scene* scene) const {
    // Push the quality settings that drive the soft-shadow technique.
    forwardShader.setFloat("u_penumbraScale", m_settings.pcssEnabled ? m_settings.penumbraScale : 0.0f);
    forwardShader.setFloat("u_cascadeBlend", m_settings.cascadeBlend);
    forwardShader.setFloat("u_shadowMapSize", static_cast<float>(m_settings.shadowMapSize));
    forwardShader.setFloat("u_minDepthBias", m_settings.minDepthBias);
    forwardShader.setFloat("u_maxDepthBias", m_settings.maxDepthBias);
    forwardShader.setFloat("u_normalBias", m_settings.normalBias);
    forwardShader.setFloat("u_texelSpacing", m_settings.texelSpacing);
    forwardShader.setInt("u_pcfKernel", m_settings.pcfKernel);

    // --- Directional CSM ---
    if (m_directionalActive && m_directionalCaster && m_directionalCaster->valid()) {
        forwardShader.setBool("u_shadowsEnabled", true);
        forwardShader.setInt("u_cascadeCount", m_settings.cascadeCount);

        // Bind the 2D array to texture unit 5.
        m_directionalCaster->bindForReading(5);
        forwardShader.setInt("u_shadowMap", 5);

        // Upload cascade split planes + light space matrices.
        int i = 0;
        for (; i < m_settings.cascadeCount && i < 4; ++i) {
            forwardShader.setFloat("u_cascadeSplitPlanes[" + std::to_string(i) + "]",
                                   m_splitDepths[i]);
            forwardShader.setMat4("u_shadowMatrices[" + std::to_string(i) + "]",
                                  m_lightSpaceMatrices[i]);
        }
        for (; i < 4; ++i) {
            forwardShader.setFloat("u_cascadeSplitPlanes[" + std::to_string(i) + "]", 0.0f);
        }
    }
    else {
        forwardShader.setBool("u_shadowsEnabled", false);
    }

    // --- Point shadows ---
    const int pointCount = static_cast<int>(m_pointLightPositions.size());
    forwardShader.setInt("u_pointShadowCount", pointCount);
    for (int i = 0; i < pointCount && i < 4; ++i) {
        if (!m_pointCasters[i]) continue;
        m_pointCasters[i]->bindForReading(6 + i);
        forwardShader.setInt("u_pointShadowMaps[" + std::to_string(i) + "]", 6 + i);
        forwardShader.setFloat("u_pointShadowFarPlanes[" + std::to_string(i) + "]",
                               m_pointLightFarPlanes[i]);
        forwardShader.setVec3("u_pointShadowPositions[" + std::to_string(i) + "]",
                              m_pointLightPositions[i]);
    }
    for (int i = pointCount; i < 4; ++i) {
        forwardShader.setInt("u_pointShadowMaps[" + std::to_string(i) + "]", -1);
    }

    if (scene) {
        auto lights = scene->FindComponentsOfType<Lindo::Components::Light::PointLight>();
        int activeIndex = 0;
        for (auto* light : lights) {
            if (!light || !light->enabled) continue;
            int shadowIndex = pointShadowIndexFor(light);
            if (activeIndex < 32) {
                forwardShader.setInt("u_pointShadowIndex[" + std::to_string(activeIndex) + "]", shadowIndex);
            }
            activeIndex++;
        }
        for (int i = activeIndex; i < 32; ++i) {
            forwardShader.setInt("u_pointShadowIndex[" + std::to_string(i) + "]", -1);
        }
    }

    // --- Spot shadows ---
    if (m_spotActive && m_spotCaster && m_spotCaster->valid()) {
        m_spotCaster->bindForReading(10);
        forwardShader.setInt("u_spotShadowMap", 10);
        forwardShader.setMat4("u_spotShadowMatrix", m_spotLightSpaceMatrix);
        forwardShader.setVec3("u_spotLightPosition", m_spotLightPosition);
        forwardShader.setVec3("u_spotLightDirection", m_spotLightDirection);
        forwardShader.setFloat("u_spotShadowFarPlane", m_spotLightFarPlane);
        forwardShader.setBool("u_spotShadowsEnabled", true);
    } else {
        forwardShader.setBool("u_spotShadowsEnabled", false);
    }
}

unsigned int ShadowManager::directionalShadowTexture() const {
    return m_directionalCaster ? m_directionalCaster->textureID() : 0;
}

int ShadowManager::pointShadowIndexFor(const Components::Light::PointLight* light) const {
    if (!light) return -1;
    for (int i = 0; i < static_cast<int>(m_pointLightSources.size()); ++i) {
        if (m_pointLightSources[i] == light) {
            return i;
        }
    }
    return -1;
}

int ShadowManager::cascadeCount() const {
    return m_settings.cascadeCount;
}

} // namespace Graphics
} // namespace Lindo
