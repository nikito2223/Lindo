#include "Renderer.h"
#include "DebugDraw.h"
#include "core/SceneManager.h"
#include "graphics/ui/UIManager.h"
#include "debug/DebugOverlay.h"
#include "graphics/skybox/skybox.h"
#include "graphics/Shadow/ShadowManager.h"
#include "core/Types/Settings.h"
#include "core/AssetManager.h"
#include "Debug/DebugLogger.h"

#include <glm/gtc/matrix_transform.hpp>
#include <world/Scene.h>
#include <Component/GameObject/GameObject.h>
#include <Component/PlayerController/Player.h>
#include <Component/Camera/Camera.h>
#include <Component/Graphics/Light.h>
#include <Component/Physhcs/MeshRenderer.h>
#include <filesystem>

#include "Core/RenderCommand.h"
#include <Debug/DebugSystem.h>
#include "Framebuffer.h"

namespace Lindo {
    namespace Graphics {

        static Lindo::Components::Rendering::Camera* GetMainCamera(Lindo::SceneManager* sceneManager) {
            if (!sceneManager) return nullptr;
            auto* currentScene = sceneManager->GetCurrentScene();
            if (!currentScene) return nullptr;

            auto cameras = currentScene->FindComponentsOfType<Lindo::Components::Rendering::Camera>();
            for (auto* cam : cameras) {
                if (cam && cam->gameObject) return cam;
            }

            auto* player = sceneManager->FindComponentInScene<Lindo::Components::Controller::Player>();
            if (player && player->gameObject) {
                return player->gameObject->getComponent<Lindo::Components::Rendering::Camera>();
            }

            return nullptr;
        }

        Renderer::Renderer(Lindo::SceneManager* scene, Lindo::Graphics::UI::UIManager* ui, Lindo::Debug::DebugSystem* debugSystem)
            : m_sceneManager(scene), m_uiManager(ui), m_debugSystem(debugSystem) {
            LOG_INFO("[Renderer] Renderer instance created successfully.");
        }

        Renderer::~Renderer() = default;

        bool Renderer::setSkybox(const std::string& hdrRelativePath, int faceResolution) {
            auto& assets = AssetManager::get();
            std::string finalPath = hdrRelativePath;
        
            // 1. Если передано просто имя (напр. "night_sky"), добавляем "skybox/"
            if (finalPath.find('/') == std::string::npos && finalPath.find('\\') == std::string::npos) {
                finalPath = "skybox/" + finalPath;
            }
        
            // 2. Если нет расширения, по умолчанию добавляем .hdr
            if (!std::filesystem::path(finalPath).has_extension()) {
                finalPath += ".hdr";
            }
        
            // Резолвим путь через AssetManager
            std::string hdrPath = assets.resolvePath(finalPath, "textures");
        
            if (!std::filesystem::exists(hdrPath)) {
                // Запасная попытка через getTexturePath / прямое разрешение
                hdrPath = assets.resolvePath(hdrRelativePath, "textures");
            }
        
            if (!std::filesystem::exists(hdrPath)) {
                LOG_WARN("[Renderer] Skybox HDR file missing at path: " + hdrRelativePath + " (" + hdrPath + ")");
                return false;
            }
        
            LOG_INFO("[Renderer] Loading Skybox HDR texture (" + std::to_string(faceResolution) + "x" + std::to_string(faceResolution) + "): " + hdrPath);
        
            try {
                m_skybox = std::make_unique<Skybox>(hdrPath, faceResolution);
                LOG_INFO("[Renderer] New HDR Skybox successfully loaded.");
                return true;
            } catch (const std::exception& e) {
                LOG_ERROR("[Renderer] Failed to load Skybox: " + std::string(e.what()));
                return false;
            }
        }

        void Renderer::init() {
            if (m_initialized) {
                LOG_WARN("[Renderer] init() called more than once; keeping existing resources.");
                return;
            }

            LOG_INFO("[Renderer] Initializing Renderer shaders, buffers, and resources...");

            DisplaySettings& startupDisplaySettings = DisplaySettings::getInstance();
            m_viewportWidth = startupDisplaySettings.windowWidth;
            m_viewportHeight = startupDisplaySettings.windowHeight;

            m_fbo = std::make_unique<Framebuffer>(m_viewportWidth, m_viewportHeight);

            RenderCommand::SetClearColor(glm::vec4(0.1f, 0.1f, 0.1f, 1.0f));

            auto& assets = AssetManager::get();
            auto& settings = Settings::getInstance();

            std::string vertPath = assets.getShaderPath("VertexShader.vert");
            std::string fragPath = assets.getShaderPath("FragmentShader.frag");

            LOG_INFO("[Renderer] Loading main lighting shaders...");
            if (!std::filesystem::exists(vertPath) || !std::filesystem::exists(fragPath)) {
                LOG_ERROR("[Renderer] Lighting shaders not found at: " + vertPath + " | " + fragPath);
            }

            m_lightingShader = std::make_unique<Shader>(vertPath.c_str(), fragPath.c_str());
            if (!m_lightingShader->isValid()) {
                LOG_ERROR("[Renderer] Main lighting shader is invalid; scene rendering is disabled.");
                return;
            }

            LOG_INFO("[Renderer] Initializing DebugDraw primitive renderer...");
            DebugDraw::GetInstance().init();

            LOG_INFO("[Renderer] Initializing ShadowManager...");
            m_shadowManager = std::make_unique<ShadowManager>();
            m_shadowManager->setQuality(settings.shadowQuality);

            m_initialized = true;
            LOG_INFO("[Renderer] ShadowManager setup complete according to Settings.");
        }

        void Renderer::render() {
            if (!m_initialized || !m_sceneManager || !m_lightingShader || !m_lightingShader->isValid()) return;

            auto* currentScene = m_sceneManager->GetCurrentScene();
            if (!currentScene) return;

            Settings& settings = Settings::getInstance();
            auto* mainCamera = GetMainCamera(m_sceneManager);

            if (mainCamera) {
                m_frustum.update(mainCamera->getProjectionMatrix() * mainCamera->getViewMatrix());
            }

            glPolygonMode(GL_FRONT_AND_BACK, settings.wireframeMode ? GL_LINE : GL_FILL);

            // PASS 1: Shadow Pass
            if (m_shadowManager && settings.enableShadows) {
                glm::mat4 viewProj = glm::mat4(1.0f);
                float nearPlane = settings.nearPlane;
                float farPlane = settings.farPlane;

                if (mainCamera) {
                    viewProj = mainCamera->getProjectionMatrix() * mainCamera->getViewMatrix();
                    nearPlane = mainCamera->getNearPlane();
                    farPlane = mainCamera->getFarPlane();
                }

                m_shadowManager->renderShadows(currentScene, viewProj, nearPlane, farPlane);
            }

            // --- БИНДИМ НАШ FBO ДЛЯ РЕНДЕРА СЦЕНЫ ---
            if (m_fbo) {
                m_fbo->bind();
            }

            RenderCommand::SetViewport(0, 0, m_viewportWidth, m_viewportHeight);
            RenderCommand::SetDepthTest(true);
            RenderCommand::SetDepthWrite(true);
            RenderCommand::SetDepthFuncLess();
            RenderCommand::SetCullFace(true, true);
            RenderCommand::SetBlending(false);

            RenderCommand::Clear(true, true);

            m_lightingShader->use();

            m_lightingShader->setFloat("gamma", settings.gamma);
            m_lightingShader->setFloat("exposure", settings.exposure);
            m_lightingShader->setInt("enableShadows", settings.enableShadows ? 1 : 0);
            m_lightingShader->setInt("enableBloom", settings.enableBloom ? 1 : 0);
            m_lightingShader->setInt("enableSSAO", settings.enableSSAO ? 1 : 0);

            if (mainCamera) {
                m_lightingShader->setMat4("viewMatrix", mainCamera->getViewMatrix());
                m_lightingShader->setMat4("projectionMatrix", mainCamera->getProjectionMatrix());

                if (auto* camgameObject = mainCamera->gameObject) {
                    m_lightingShader->setVec3("viewPos", camgameObject->getWorldPosition());
                }
            }

            if (m_shadowManager && settings.enableShadows) {
                m_shadowManager->bindShadowTextures(*m_lightingShader, currentScene);
            }

            renderScene(*currentScene, *m_lightingShader, mainCamera);

            // Skybox Pass
            if (mainCamera && m_skybox) {
                glm::mat4 view = mainCamera->getViewMatrix();
                glm::mat4 skyboxView = glm::mat4(glm::mat3(view));

                float time = static_cast<float>(glfwGetTime()) * 0.015f;
                skyboxView = glm::rotate(skyboxView, time, glm::vec3(0.0f, 1.0f, 0.0f));

                glm::mat4 projection = mainCamera->getProjectionMatrix();

                RenderCommand::SetDepthFuncLEqual();
                RenderCommand::SetDepthWrite(false);
                RenderCommand::SetCullFace(false);

                if (auto* camObj = mainCamera->gameObject) {
                    m_skybox->Draw(skyboxView, projection, static_cast<float>(glfwGetTime()), camObj->getWorldPosition());
                }

                RenderCommand::SetDepthWrite(true);
                RenderCommand::SetCullFace(true, true);
                RenderCommand::SetDepthFuncLess();
            }

            // Physics & Gizmo Debug Draw
            bool renderPhysicsDebug = settings.isDebugDrawEnabled();
            if (renderPhysicsDebug && currentScene) {
                auto& debugDraw = DebugDraw::GetInstance();
                glm::mat4 view = mainCamera ? mainCamera->getViewMatrix() : glm::mat4(1.0f);
                glm::mat4 projection = mainCamera ? mainCamera->getProjectionMatrix() : glm::mat4(1.0f);

                debugDraw.begin(view, projection);
                RenderCommand::SetDepthTest(false);

                auto* dirLight = currentScene->FindComponentOfType<Lindo::Components::Light::DirectionalLight>();
                if (dirLight && dirLight->enabled) {
                    glm::vec3 pos = dirLight->getPosition();
                    glm::vec3 dir = glm::normalize(dirLight->direction);
                    glm::vec3 color = glm::vec3(dirLight->color);

                    debugDraw.DrawSphere(pos, 0.6f, color, 12);
                    debugDraw.DrawArrow(pos, pos + dir * 5.0f, color, 0.5f);
                }

                auto pointLights = currentScene->FindComponentsOfType<Lindo::Components::Light::PointLight>();
                for (auto* pointLight : pointLights) {
                    if (!pointLight || !pointLight->enabled) continue;
                    debugDraw.DrawSphere(pointLight->getPosition(), 0.35f, glm::vec3(pointLight->color), 12);
                }

                const auto& gameObjects = currentScene->GetGameObjects();
                for (size_t i = 0; i < gameObjects.size(); ++i) {
                    if (gameObjects[i] && gameObjects[i]->isActive) {
                        gameObjects[i]->DrawGizmos();
                    }
                }

                debugDraw.render();
                RenderCommand::SetDepthTest(true);
            }

            if (m_fbo) {
                m_fbo->unbind();
            }

            RenderCommand::SetViewport(0, 0, m_viewportWidth, m_viewportHeight);
            if (m_fbo) {
                glBindFramebuffer(GL_READ_FRAMEBUFFER, m_fbo->getFBOID());
                GLenum framebufferStatus = glCheckFramebufferStatus(GL_READ_FRAMEBUFFER);

                RenderCommand::SetDepthTest(false);
                RenderCommand::SetCullFace(false);
                RenderCommand::SetBlending(true);

                glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

                if (framebufferStatus == GL_FRAMEBUFFER_COMPLETE) {
                    glBlitFramebuffer(
                        0, 0, m_viewportWidth, m_viewportHeight,
                        0, 0, m_viewportWidth, m_viewportHeight,
                        GL_COLOR_BUFFER_BIT, GL_LINEAR
                    );
                }

                if (m_uiManager) {
                    m_uiManager->update();
                    m_uiManager->render();
                }
            }

            if (m_debugSystem) {
                m_debugSystem->update(m_sceneManager, mainCamera);
                m_debugSystem->renderUI(m_uiManager);
            }
        }

        void Renderer::applySettings(const Lindo::Settings& settings) {
            if (m_shadowManager) {
                m_shadowManager->setQuality(settings.shadowQuality);
            }
        }

        void Renderer::renderScene(Lindo::World::Scene& scene, Shader& shader, Lindo::Components::Rendering::Camera* camera) {
            shader.use();

            shader.setInt("activePointLights", 0);
            shader.setBool("spotLight.enabled", false);

            if (camera && camera->gameObject) {
                shader.setMat4("viewMatrix", camera->getViewMatrix());
                shader.setMat4("projectionMatrix", camera->getProjectionMatrix());
                shader.setVec3("viewPos", camera->gameObject->getWorldPosition());
            }

            auto* sunObject = scene.FindGameObject("Sun");
            auto* sunLight = sunObject
                ? sunObject->getComponent<Lindo::Components::Light::DirectionalLight>()
                : nullptr;
            if (sunLight && sunLight->enabled) {
                sunLight->ApplyToShader(shader, "dirLight");
            } else {
                shader.setBool("dirLight.enabled", false);
            }

            constexpr int maxPointLights = 32;
            int activeLights = 0;
            for (auto* pointLight : scene.FindComponentsOfType<Lindo::Components::Light::PointLight>()) {
                if (!pointLight || !pointLight->enabled || activeLights >= maxPointLights) continue;
                pointLight->ApplyToShader(shader, "pointLights[" + std::to_string(activeLights) + "]");
                ++activeLights;
            }
            shader.setInt("activePointLights", activeLights);

            if (auto* spotLight = scene.FindComponentOfType<Lindo::Components::Light::SpotLight>();
                spotLight && spotLight->enabled) {
                spotLight->ApplyToShader(shader, "spotLight");
            }

            for (const auto& object : scene.GetGameObjects()) {
                if (!object || !object->isActive) continue;
                auto* meshRenderer = object->getComponent<Lindo::Components::Physics::MeshRenderer>();
                if (meshRenderer && meshRenderer->IsEnabled()) {
                    if (meshRenderer->model) {
                        const auto localBounds = meshRenderer->model->getAABB();
                        const glm::mat4 world = object->getWorldMatrix();
                        glm::vec3 min = glm::vec3(std::numeric_limits<float>::max());
                        glm::vec3 max = glm::vec3(std::numeric_limits<float>::lowest());
                        for (const glm::vec3& corner : {
                            glm::vec3(localBounds.min.x, localBounds.min.y, localBounds.min.z),
                            glm::vec3(localBounds.max.x, localBounds.min.y, localBounds.min.z),
                            glm::vec3(localBounds.min.x, localBounds.max.y, localBounds.min.z),
                            glm::vec3(localBounds.max.x, localBounds.max.y, localBounds.min.z),
                            glm::vec3(localBounds.min.x, localBounds.min.y, localBounds.max.z),
                            glm::vec3(localBounds.max.x, localBounds.min.y, localBounds.max.z),
                            glm::vec3(localBounds.min.x, localBounds.max.y, localBounds.max.z),
                            glm::vec3(localBounds.max.x, localBounds.max.y, localBounds.max.z) }) {
                            const glm::vec3 transformed = glm::vec3(world * glm::vec4(corner, 1.0f));
                            min = glm::min(min, transformed);
                            max = glm::max(max, transformed);
                        }
                        if (!m_frustum.intersects(Lindo::Math::AABB(min, max))) continue;
                    }
                    meshRenderer->OnDraw(shader);
                }
            }
        }

        void Renderer::onResize(int width, int height) {
            if (width <= 0 || height <= 0) return;
            LOG_INFO("[Renderer] Viewport resized: " + std::to_string(width) + "x" + std::to_string(height));
            m_viewportWidth = width;
            m_viewportHeight = height;

            if (m_fbo) {
                m_fbo->resize(width, height);
            }

            auto* mainCamera = GetMainCamera(m_sceneManager);
            if (mainCamera) {
                mainCamera->setAspectRatio(static_cast<float>(width) / static_cast<float>(height));
            }
        }
    }
}