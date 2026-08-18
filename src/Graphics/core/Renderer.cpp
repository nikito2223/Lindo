#include "Renderer.h"
#include "DebugDraw.h"
#include "core/SceneManager.h"
#include "graphics/ui/UIManager.h"
#include "debug/DebugOverlay.h"
#include "graphics/render/skybox.h"
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
#include <Physics/Collider/Collider.h>
#include <filesystem>

#include "Core/RenderCommand.h"
#include <Physics/Collider/MeshCollider.h>
#include <Physics/Collider/CapsuleCollider.h>
#include <Physics/Collider/BoxCollider.h>
#include <Physics/Collider/SphereCollider.h>
#include <Debug/DebugSystem.h>

namespace Lindo {
    namespace Graphics {

        static Lindo::Components::Rendering::Camera* GetMainCamera(Lindo::SceneManager* sceneManager) {
            if (!sceneManager) return nullptr;

            auto* currentScene = sceneManager->GetCurrentScene();
            if (!currentScene) return nullptr;

            auto cameras = currentScene->FindComponentsOfType<Lindo::Components::Rendering::Camera>();
            for (auto* cam : cameras) {
                if (cam && cam->gameObject) {
                    return cam;
                }
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

        void Renderer::init() {
            LOG_INFO("[Renderer] Initializing Renderer shaders, buffers, and resources...");

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
            LOG_INFO("[Renderer] Main lighting shader initialized successfully.");

            std::string hdrPath = assets.resolvePath("skybox/qwantani_dusk_2_puresky_4k.hdr", "textures");
            if (std::filesystem::exists(hdrPath)) {
                LOG_INFO("[Renderer] Skybox file found. Loading HDR texture: " + hdrPath);
                m_skybox = std::make_unique<Skybox>(hdrPath, 1024);
                LOG_INFO("[Renderer] HDR Skybox successfully created.");
            }
            else {
                LOG_WARN("[Renderer] Skybox HDR file missing at path: " + hdrPath);
            }

            LOG_INFO("[Renderer] Initializing DebugDraw primitive renderer...");
            m_debugDraw = std::make_unique<DebugDraw>();
            m_debugDraw->init();

            LOG_INFO("[Renderer] Initializing ShadowManager...");
            m_shadowManager = std::make_unique<ShadowManager>();

            // Синхронизация ShadowQuality из настроек
            m_shadowManager->setQuality(settings.shadowQuality);
            LOG_INFO("[Renderer] ShadowManager setup complete according to Settings.");
        }

        void Renderer::render() {
            if (!m_sceneManager || !m_lightingShader) return;

            auto* currentScene = m_sceneManager->GetCurrentScene();
            if (!currentScene) return;

            DisplaySettings& displaySettings = DisplaySettings::getInstance();
            Settings& settings = Settings::getInstance();
            auto* mainCamera = GetMainCamera(m_sceneManager);

            // Установка режима wireframe на основе настроек
            glPolygonMode(GL_FRONT_AND_BACK, settings.wireframeMode ? GL_LINE : GL_FILL);

            // PASS 1: Shadow Pass (если тени включены)
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

            // PASS 2: Forward Main Pass
            RenderCommand::SetViewport(0, 0, displaySettings.windowWidth, displaySettings.windowHeight);
            RenderCommand::SetDepthTest(true);
            RenderCommand::SetDepthWrite(true);
            RenderCommand::SetDepthFuncLess();
            RenderCommand::SetCullFace(true, true);
            RenderCommand::SetBlending(false);

            RenderCommand::Clear(true, true);

            m_lightingShader->use();

            // Передача параметров постобработки, гаммы, экспозиции и теней в шейдер
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

            m_sceneManager->Render(*m_lightingShader);

            // PASS 3: Skybox
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

            // PASS 4: UI
            if (m_uiManager) {
                RenderCommand::SetDepthTest(false);
                RenderCommand::SetCullFace(false);
                RenderCommand::SetBlending(true);

                m_uiManager->update();
                m_uiManager->render();
            }

            // PASS 5: Physics & Gizmo Debug Draw
            bool renderPhysicsDebug = settings.isDebugDrawEnabled();
            if (m_debugDraw && renderPhysicsDebug) {
                if (currentScene) {
                    glm::mat4 view = mainCamera ? mainCamera->getViewMatrix() : glm::mat4(1.0f);
                    glm::mat4 projection = mainCamera ? mainCamera->getProjectionMatrix() : glm::mat4(1.0f);

                    m_debugDraw->begin(view, projection);

                    // 1. DIRECTIONAL LIGHT GIZMO
                    auto* dirLight = currentScene->FindComponentOfType<Lindo::Components::Light::DirectionalLight>();
                    if (dirLight && dirLight->enabled) {
                        glm::vec3 pos = dirLight->getPosition();
                        glm::vec3 dir = glm::normalize(dirLight->direction);
                        glm::vec3 color = glm::vec3(dirLight->color);

                        m_debugDraw->DrawSphere(pos, 0.6f, color, 12);
                        m_debugDraw->DrawArrow(pos, pos + dir * 5.0f, color, 0.5f);

                        glm::vec3 right = glm::normalize(glm::cross(dir, glm::abs(dir.y) > 0.99f ? glm::vec3(1.0f, 0.0f, 0.0f) : glm::vec3(0.0f, 1.0f, 0.0f)));
                        glm::vec3 up = glm::normalize(glm::cross(right, dir));
                        float offset = 1.2f;

                        m_debugDraw->DrawLine(pos + right * offset, pos + right * offset + dir * 4.0f, color);
                        m_debugDraw->DrawLine(pos - right * offset, pos - right * offset + dir * 4.0f, color);
                        m_debugDraw->DrawLine(pos + up * offset, pos + up * offset + dir * 4.0f, color);
                        m_debugDraw->DrawLine(pos - up * offset, pos - up * offset + dir * 4.0f, color);
                    }

                    // 2. POINT LIGHT GIZMO
                    auto pointLights = currentScene->FindComponentsOfType<Lindo::Components::Light::PointLight>();
                    for (auto* pointLight : pointLights) {
                        if (!pointLight || !pointLight->enabled) continue;
                        glm::vec3 pos = pointLight->getPosition();
                        glm::vec3 col = glm::vec3(pointLight->color);

                        m_debugDraw->DrawSphere(pos, 0.35f, col, 12);

                        float r = 0.8f;
                        m_debugDraw->DrawLine(pos - glm::vec3(r, 0, 0), pos + glm::vec3(r, 0, 0), col);
                        m_debugDraw->DrawLine(pos - glm::vec3(0, r, 0), pos + glm::vec3(0, r, 0), col);
                        m_debugDraw->DrawLine(pos - glm::vec3(0, 0, r), pos + glm::vec3(0, 0, r), col);
                    }

                    // 3. COLLIDERS
                    auto colliders = currentScene->FindComponentsOfType<Lindo::Components::Physics::Collider>();

                    for (auto* collider : colliders) {
                        if (!collider || !collider->gameObject) continue;

                        glm::mat4 world = collider->gameObject->getWorldMatrix();
                        glm::vec3 pos = collider->GetWorldPosition();
                        glm::vec3 xAxis = glm::vec3(world[0]) * 0.5f;
                        glm::vec3 yAxis = glm::vec3(world[1]) * 0.5f;
                        glm::vec3 zAxis = glm::vec3(world[2]) * 0.5f;

                        m_debugDraw->DrawLine(pos, pos + xAxis, glm::vec3(1.0f, 0.0f, 0.0f));
                        m_debugDraw->DrawLine(pos, pos + yAxis, glm::vec3(0.0f, 1.0f, 0.0f));
                        m_debugDraw->DrawLine(pos, pos + zAxis, glm::vec3(0.0f, 0.0f, 1.0f));

                        // Важно: MeshCollider проверяется Первым, так как наследуется от BoxCollider
                        if (auto* mesh = dynamic_cast<Lindo::Components::Physics::MeshCollider*>(collider)) {
                            glm::mat4 transform = glm::translate(world, mesh->GetOffset());
                            transform = glm::scale(transform, mesh->GetSize());
                            m_debugDraw->DrawWireBox(transform, glm::vec3(1.0f, 0.0f, 1.0f));
                        }
                        else if (auto* box = dynamic_cast<Lindo::Components::Physics::BoxCollider*>(collider)) {
                            glm::mat4 transform = glm::translate(world, box->GetOffset());
                            transform = glm::scale(transform, box->GetSize());
                            m_debugDraw->DrawWireBox(transform, glm::vec3(0.0f, 1.0f, 1.0f));
                        }
                        else if (auto* sphere = dynamic_cast<Lindo::Components::Physics::SphereCollider*>(collider)) {
                            float maxScale = glm::max(glm::max(glm::length(glm::vec3(world[0])), glm::length(glm::vec3(world[1]))), glm::length(glm::vec3(world[2])));
                            float worldRadius = sphere->GetRadius() * maxScale;

                            glm::mat4 transform = glm::translate(glm::mat4(1.0f), pos);
                            transform = glm::scale(transform, glm::vec3(worldRadius));
                            m_debugDraw->DrawWireSphereFast(transform, glm::vec3(0.0f, 1.0f, 1.0f));
                        }
                        else if (auto* capsule = dynamic_cast<Lindo::Components::Physics::CapsuleCollider*>(collider)) {
                            glm::vec3 top, bottom;
                            capsule->GetEndpoints(bottom, top);
                            float radius = capsule->GetWorldRadius();
                            glm::vec3 axis = top - bottom;
                            float axisLen = glm::length(axis);
                            glm::vec3 dir = axisLen > 1e-6f ? axis / axisLen : glm::vec3(0.0f, 1.0f, 0.0f);

                            glm::vec3 ortho = glm::abs(glm::dot(dir, glm::vec3(0.0f, 1.0f, 0.0f))) > 0.99f
                                ? glm::vec3(1.0f, 0.0f, 0.0f)
                                : glm::normalize(glm::cross(dir, glm::vec3(0.0f, 1.0f, 0.0f)));
                            glm::vec3 tangent = glm::normalize(glm::cross(dir, ortho));
                            glm::vec3 bitangent = glm::normalize(glm::cross(dir, tangent));

                            const int segments = 20;
                            const int halfSegments = 10;
                            const glm::vec3 color(1.0f, 0.7f, 0.0f);

                            auto drawCircle = [&](const glm::vec3& center) {
                                m_debugDraw->DrawCircle(center, tangent, bitangent, radius, color, segments);
                                };
                            drawCircle(top);
                            drawCircle(bottom);

                            m_debugDraw->DrawLine(top + tangent * radius, bottom + tangent * radius, color);
                            m_debugDraw->DrawLine(top - tangent * radius, bottom - tangent * radius, color);
                            m_debugDraw->DrawLine(top + bitangent * radius, bottom + bitangent * radius, color);
                            m_debugDraw->DrawLine(top - bitangent * radius, bottom - bitangent * radius, color);

                            auto drawArc = [&](const glm::vec3& center, const glm::vec3& planeVec, float sign) {
                                glm::vec3 prevPoint = center + planeVec * radius;
                                for (int i = 1; i <= halfSegments; ++i) {
                                    float theta = 3.14159265f * float(i) / float(halfSegments);
                                    glm::vec3 nextPoint = center + cos(theta) * planeVec * radius + sin(theta) * dir * radius * sign;
                                    m_debugDraw->DrawLine(prevPoint, nextPoint, color);
                                    prevPoint = nextPoint;
                                }
                                };

                            drawArc(top, tangent, 1.0f);
                            drawArc(top, bitangent, 1.0f);
                            drawArc(bottom, tangent, -1.0f);
                            drawArc(bottom, bitangent, -1.0f);
                        }
                    }

                    m_debugDraw->render();
                }
            }

            // PASS 6: Debug Overlay (если включен показ FPS / дебага)
            if (m_debugSystem) {
                m_debugSystem->update(m_sceneManager, mainCamera);
                m_debugSystem->renderUI(m_uiManager);
            }
        }

        void Renderer::onResize(int width, int height) {
            if (height == 0) height = 1;

            LOG_INFO("[Renderer] Viewport resized: " + std::to_string(width) + "x" + std::to_string(height));

            auto* mainCamera = GetMainCamera(m_sceneManager);
            if (mainCamera) {
                mainCamera->setAspectRatio(static_cast<float>(width) / static_cast<float>(height));
            }

            RenderCommand::SetViewport(0, 0, width, height);
        }
    }
}