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
#include <Component/Graphics/MeshRenderer.h>
#include <filesystem>
#include <unordered_set>
#include <vector>

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

        Renderer::~Renderer() {
            if (m_sceneSamplesQuery != 0) {
                glDeleteQueries(1, &m_sceneSamplesQuery);
            }
        }

        bool Renderer::setSkybox(const std::string& hdrRelativePath, int faceResolution) {
            auto& assets = AssetManager::get();

            // 1. Нормализуем слэши и удаляем лишний префикс assets/ или ..\assets\ если он передался
            std::string clean = hdrRelativePath;
            std::replace(clean.begin(), clean.end(), '\\', '/');
        
            std::string marker = "assets/";
            size_t pos = clean.find(marker);
            if (pos != std::string::npos) {
                clean = clean.substr(pos + marker.length());
            }
        
            // 2. Добавляем расширение, если его нет
            std::filesystem::path pathObj(clean);
            if (!pathObj.has_extension()) {
                pathObj.replace_extension(".hdr");
            }
        
            // 3. Резолвим путь. Передаем только чистое имя файла или подпуть
            std::string hdrPath = assets.resolvePath(pathObj.string(), "skybox");
            if (m_skybox && m_skyboxPath == hdrPath && m_skyboxResolution == faceResolution) {
                return true;
            }
        
            // 4. Проверяем существование файла
            if (!std::filesystem::exists(hdrPath)) {
                LOG_WARN("[Renderer] Skybox HDR file missing at path: " + hdrRelativePath + " (resolved to: " + hdrPath + ")");
                return false;
            }
        
            LOG_INFO("[Renderer] Loading Skybox HDR texture (" + std::to_string(faceResolution) + "x" + std::to_string(faceResolution) + "): " + hdrPath);
        
            try {
                m_skybox = std::make_unique<Skybox>(hdrPath, faceResolution);
                m_skyboxPath = hdrPath;
                m_skyboxResolution = faceResolution;
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
            glGenQueries(1, &m_sceneSamplesQuery);
            if (m_sceneSamplesQuery == 0) {
                LOG_ERROR("[Scene Diagnostics] Could not create an OpenGL samples-passed query; fragment visibility checks are unavailable.");
            }

            RenderCommand::SetClearColor(glm::vec4(0.1f, 0.1f, 0.1f, 1.0f));

            auto& assets = AssetManager::get();
            auto& settings = Settings::getInstance();

            std::string vertPath = assets.getShaderPath("Main.gslv");
            std::string fragPath = assets.getShaderPath("Main.gslf");

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
            if (!m_initialized || !m_sceneManager) return;
            if (!m_lightingShader || !m_lightingShader->isValid()) {
                static bool shaderFailureReported = false;
                if (!shaderFailureReported) {
                    shaderFailureReported = true;
                    LOG_ERROR("[Scene Diagnostics] Scene rendering is blocked: the main lighting shader is missing or invalid.");
                }
                return;
            }

            auto* currentScene = m_sceneManager->GetCurrentScene();
            if (!currentScene) return;

            if (m_sceneSamplesPending) {
                GLint queryAvailable = GL_FALSE;
                glGetQueryObjectiv(m_sceneSamplesQuery, GL_QUERY_RESULT_AVAILABLE, &queryAvailable);
                if (queryAvailable == GL_TRUE) {
                    GLuint passedSamples = 0;
                    glGetQueryObjectuiv(m_sceneSamplesQuery, GL_QUERY_RESULT, &passedSamples);
                    const SceneRenderStats& queriedStats = m_pendingQueryStats;
                    if (queriedStats.drawCalls > 0) {
                        const std::string sampleState = m_pendingQueryScene + (passedSamples == 0 ? ":zero-samples" : ":visible-samples");
                        if (sampleState != m_lastSampleDiagnostic) {
                            m_lastSampleDiagnostic = sampleState;
                            if (passedSamples == 0) {
                                LOG_ERROR("[Scene Diagnostics] Scene '" + m_pendingQueryScene + "' submitted " +
                                    std::to_string(queriedStats.drawCalls) + " mesh draw call(s) (" +
                                    std::to_string(queriedStats.triangles) + " triangles), but 0 fragments passed depth/stencil. "
                                    "Check camera matrices, vertex transforms, winding/culling, and depth state.");
                            } else {
                                LOG_INFO("[Scene Diagnostics] Scene '" + m_pendingQueryScene + "' rasterized " +
                                    std::to_string(passedSamples) + " sample(s) from " +
                                    std::to_string(queriedStats.drawCalls) + " mesh draw call(s).");
                            }
                        }
                    }
                    m_sceneSamplesPending = false;
                }
            }

            Settings& settings = Settings::getInstance();
            auto* mainCamera = GetMainCamera(m_sceneManager);

            static std::unordered_map<std::string, bool> cameraStateByScene;
            const std::string sceneName = currentScene->GetName();
            const bool cameraFound = mainCamera != nullptr;
            auto [cameraState, firstObservation] = cameraStateByScene.emplace(sceneName, cameraFound);
            if (firstObservation || cameraState->second != cameraFound) {
                cameraState->second = cameraFound;
                if (cameraFound) {
                    const glm::vec3 cameraPosition = mainCamera->getPosition();
                    const glm::vec3 cameraFront = mainCamera->getFront();
                    LOG_INFO("[Renderer] Camera found in scene '" + sceneName +
                        "' position=(" + std::to_string(cameraPosition.x) + ", " +
                        std::to_string(cameraPosition.y) + ", " + std::to_string(cameraPosition.z) +
                        ") front=(" + std::to_string(cameraFront.x) + ", " +
                        std::to_string(cameraFront.y) + ", " + std::to_string(cameraFront.z) + ")");
                } else {
                    LOG_WARN("[Renderer] No Camera component found in scene '" + sceneName + "'.");
                }
            }

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

            if (m_shadowManager) {
                m_shadowManager->bindShadowTextures(*m_lightingShader, currentScene);
            }

            if (mainCamera) {
                const bool queryStarted = m_sceneSamplesQuery != 0 && !m_sceneSamplesPending;
                if (queryStarted) {
                    glBeginQuery(GL_SAMPLES_PASSED, m_sceneSamplesQuery);
                }

                const SceneRenderStats frameStats = renderScene(*currentScene, *m_lightingShader, mainCamera);

                if (queryStarted) {
                    glEndQuery(GL_SAMPLES_PASSED);
                    m_sceneSamplesPending = true;
                    m_pendingQueryScene = sceneName;
                    m_pendingQueryStats = frameStats;
                }

                if (settings.debugMode && frameStats.drawCalls > 0 && m_pixelProbeScene != sceneName &&
                    m_fbo && m_fbo->isComplete()) {
                    m_pixelProbeScene = sceneName;
                    std::vector<unsigned char> pixels(static_cast<std::size_t>(m_viewportWidth) *
                        static_cast<std::size_t>(m_viewportHeight) * 4);
                    glReadPixels(0, 0, m_viewportWidth, m_viewportHeight, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
                    const GLenum readError = glGetError();
                    if (readError != GL_NO_ERROR) {
                        LOG_ERROR("[Scene Diagnostics] FBO pixel probe failed with OpenGL error " +
                            std::to_string(static_cast<unsigned int>(readError)) + ".");
                    } else {
                        std::size_t changedPixels = 0;
                        for (std::size_t pixel = 0; pixel < pixels.size(); pixel += 4) {
                            if (pixels[pixel] < 23 || pixels[pixel] > 29 ||
                                pixels[pixel + 1] < 23 || pixels[pixel + 1] > 29 ||
                                pixels[pixel + 2] < 23 || pixels[pixel + 2] > 29) {
                                ++changedPixels;
                            }
                        }

                        if (changedPixels == 0) {
                            LOG_ERROR("[Scene Diagnostics] Scene '" + sceneName + "' issued mesh draws, but every FBO pixel "
                                "is within 3/255 per channel of the clear color (RGB 26,26,26). The scene produced no "
                                "meaningful visible color change.");
                        } else {
                            LOG_INFO("[Scene Diagnostics] FBO pixel probe for scene '" + sceneName + "': " +
                                std::to_string(changedPixels) + " of " +
                                std::to_string(pixels.size() / 4) + " pixels differ from the clear color.");
                        }
                    }
                }
            } else {
                std::size_t activeMeshRenderers = 0;
                for (const auto& object : currentScene->GetGameObjects()) {
                    if (!object || !object->isActive) continue;
                    auto* meshRenderer = object->getComponent<Lindo::Components::Physics::MeshRenderer>();
                    if (meshRenderer && meshRenderer->IsEnabled()) ++activeMeshRenderers;
                }

                if (activeMeshRenderers == 0) {
                    if (m_missingCameraScene != sceneName) {
                        LOG_INFO("[Scene Diagnostics] Scene '" + sceneName + "' has no camera or active 3D renderers; this is a UI-only scene.");
                    }
                    m_missingCameraScene = sceneName;
                    m_missingCameraFrames = 0;
                } else {
                    if (m_missingCameraScene != sceneName) {
                        m_missingCameraScene = sceneName;
                        m_missingCameraFrames = 0;
                        LOG_WARN("[Scene Diagnostics] Scene '" + sceneName + "' has " +
                            std::to_string(activeMeshRenderers) + " active 3D renderer(s) but no camera; drawing is skipped.");
                    }
                    ++m_missingCameraFrames;
                    if (m_missingCameraFrames == 120) {
                        LOG_ERROR("[Scene Diagnostics] Scene '" + sceneName + "' still has " +
                            std::to_string(activeMeshRenderers) + " active 3D renderer(s) but no camera after 120 render frames.");
                    }
                }
            }
            if (mainCamera) {
                m_missingCameraScene.clear();
                m_missingCameraFrames = 0;
            }

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
                    Shader::logOpenGLErrors("scene framebuffer blit to window");
                } else {
                    static GLenum lastIncompleteStatus = GL_FRAMEBUFFER_UNDEFINED;
                    if (lastIncompleteStatus != framebufferStatus) {
                        lastIncompleteStatus = framebufferStatus;
                        LOG_ERROR("[Scene Diagnostics] Scene framebuffer was not presented: read FBO " +
                            std::to_string(m_fbo->getFBOID()) + " has status " +
                            std::to_string(static_cast<unsigned int>(framebufferStatus)) + ".");
                    }
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

        Renderer::SceneRenderStats Renderer::renderScene(Lindo::World::Scene& scene, Shader& shader, Lindo::Components::Rendering::Camera* camera) {
            SceneRenderStats stats;
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
                    ++stats.meshRenderers;

                    bool hasIndexedGeometry = false;
                    std::size_t objectDrawCalls = 0;
                    std::size_t objectTriangles = 0;
                    auto collectMeshStats = [&](const Lindo::Graphics::Mesh& mesh) {
                        if (mesh.indices.empty()) return;
                        hasIndexedGeometry = true;
                        ++objectDrawCalls;
                        objectTriangles += mesh.indices.size() / 3;
                    };

                    if (meshRenderer->mesh) {
                        collectMeshStats(*meshRenderer->mesh);
                    } else if (meshRenderer->model) {
                        for (const auto& modelMesh : meshRenderer->model->getMeshes()) {
                            collectMeshStats(modelMesh);
                        }
                    }

                    if (!hasIndexedGeometry) {
                        ++stats.invalidRenderers;
                        static std::unordered_set<std::string> loggedInvalidGeometry;
                        const std::string key = scene.GetName() + ":" + object->getName();
                        if (loggedInvalidGeometry.insert(key).second) {
                            LOG_ERROR("[Scene Diagnostics] MeshRenderer on '" + object->getName() +
                                "' in scene '" + scene.GetName() +
                                "' has no indexed geometry; no glDrawElements call can produce pixels.");
                        }
                        continue;
                    }

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
                        if (!m_frustum.intersects(Lindo::Math::AABB(min, max))) {
                            ++stats.culledRenderers;
                            static std::unordered_set<std::string> loggedCulledObjects;
                            if (loggedCulledObjects.insert(object->getName()).second) {
                                LOG_WARN("[Renderer] Frustum culled mesh object '" + object->getName() +
                                    "' bounds min=(" + std::to_string(min.x) + ", " + std::to_string(min.y) +
                                    ", " + std::to_string(min.z) + ") max=(" + std::to_string(max.x) +
                                    ", " + std::to_string(max.y) + ", " + std::to_string(max.z) + ")");
                            }
                            continue;
                        }
                    }
                    ++stats.submittedRenderers;
                    stats.drawCalls += objectDrawCalls;
                    stats.triangles += objectTriangles;
                    static std::unordered_set<std::string> loggedSubmittedObjects;
                    if (loggedSubmittedObjects.insert(object->getName()).second) {
                        LOG_INFO("[Renderer] Submitting mesh object '" + object->getName() + "' to draw.");
                    }
                    meshRenderer->OnDraw(shader);
                    if (Settings::getInstance().debugMode) {
                        Shader::logOpenGLErrors("drawing mesh object '" + object->getName() + "'");
                    }
                }
            }

            static std::unordered_map<std::string, std::string> lastSceneStates;
            std::string state = "meshRenderers=" + std::to_string(stats.meshRenderers) +
                ", invalid=" + std::to_string(stats.invalidRenderers) +
                ", culled=" + std::to_string(stats.culledRenderers) +
                ", submitted=" + std::to_string(stats.submittedRenderers) +
                ", drawCalls=" + std::to_string(stats.drawCalls) +
                ", triangles=" + std::to_string(stats.triangles);
            if (lastSceneStates[scene.GetName()] != state) {
                lastSceneStates[scene.GetName()] = state;
                if (stats.meshRenderers == 0) {
                    LOG_INFO("[Scene Diagnostics] Scene '" + scene.GetName() + "' has no active MeshRenderer components; it may be UI-only.");
                } else if (stats.submittedRenderers == 0) {
                    LOG_ERROR("[Scene Diagnostics] No visible 3D draw submitted for scene '" + scene.GetName() + "': " + state + ".");
                } else {
                    LOG_INFO("[Scene Diagnostics] Scene '" + scene.GetName() + "': " + state + ".");
                }
            }
            return stats;
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