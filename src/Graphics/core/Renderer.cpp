#include "Renderer.h"
#include "DebugDraw.h"
#include "core/SceneManager.h"
#include "graphics/ui/UIManager.h"
#include "debug/DebugOverlay.h"
#include "graphics/render/skybox.h"
#include "graphics/Shadow/ShadowManager.h"
#include "core/Globals.h"
#include <glm/gtc/matrix_transform.hpp>
#include <world/Scene.h>
#include <Component/GameObject/GameObject.h>
#include <Component/PlayerController/Player.h>
#include <Component/Camera/Camera.h>
#include <Component/Graphics/Light.h>
#include <Physics/Collider/Collider.h>
#include <Physics/Collider/BoxCollider.h>
#include <Physics/Collider/CapsuleCollider.h>
#include <Physics/Collider/MeshCollider.h>
#include <Physics/Collider/SphereCollider.h>
#include <fstream>
#include "Debug/DebugLogger.h"

namespace Lindo {
    namespace Graphics {

        static bool fileExists(const std::string& path) {
            std::ifstream f(path);
            return f.good();
        }

        // Вспомогательная функция для чистой добычи активной камеры сцены
        static Lindo::Components::Rendering::Camera* GetMainCamera(Lindo::SceneManager* sceneManager) {
            if (!sceneManager) return nullptr;
            
            auto* currentScene = sceneManager->GetCurrentScene();
            if (!currentScene) return nullptr;

            // 1. Ищем первую камеру на сцене напрямую
            auto cameras = currentScene->FindComponentsOfType<Lindo::Components::Rendering::Camera>();
            for (auto* cam : cameras) {
                if (cam && cam->owner) {
                    return cam;
                }
            }
            
            // 2. Fallback: Если камеры на сцене нет, пробуем через игрока
            auto* player = sceneManager->FindComponentInScene<Lindo::Components::Controller::Player>();
            if (player && player->owner) {
                return player->owner->getComponent<Lindo::Components::Rendering::Camera>();
            }

            return nullptr;
        }

        Renderer::Renderer(Lindo::SceneManager* scene, Lindo::Graphics::UI::UIManager* ui, Lindo::Debug::DebugOverlay* debug)
            : m_sceneManager(scene), m_uiManager(ui), m_debugOverlay(debug) {
            LOG_INFO("Renderer instance created.");
        }

        Renderer::~Renderer() = default;

        void Renderer::init() {
            LOG_INFO("Initializing Renderer shaders and global resources...");

            std::string vertPath = std::string(Globals::pathData) + "shaders/VertexShader.vert";
            std::string fragPath = std::string(Globals::pathData) + "shaders/FragmentShader.frag";

            if (!fileExists(vertPath) || !fileExists(fragPath)) {
                LOG_ERROR("Lighting shaders not found at paths: " + vertPath + " | " + fragPath);
            }

            m_lightingShader = std::make_unique<Shader>(vertPath.c_str(), fragPath.c_str());
            LOG_INFO("Main lighting shader created successfully.");

            std::string skyboxVs = std::string(Globals::pathData) + "shaders/skybox/skybox.vs";
            std::string skyboxFs = std::string(Globals::pathData) + "shaders/skybox/skybox.fs";

            if (!fileExists(skyboxVs) || !fileExists(skyboxFs)) {
                LOG_ERROR("Skybox shaders not found at paths: " + skyboxVs + " | " + skyboxFs);
            }

            m_skyboxShader = std::make_unique<Shader>(skyboxVs.c_str(), skyboxFs.c_str());
            LOG_INFO("Skybox shader created successfully.");

            // Загрузка skybox
            std::string hdrPath = std::string(Globals::pathData) + "textures/skybox/qwantani_dusk_2_puresky_4k.hdr";
            if (fileExists(hdrPath)) {
                LOG_INFO("Loading HDR skybox texture from: " + hdrPath);
                m_skybox = std::make_unique<Skybox>(hdrPath, 1024);
                LOG_INFO("HDR Skybox successfully initialized.");
            }
            else {
                LOG_WARN("Skybox HDR file not found at path: " + hdrPath);
            }

            if (!m_skybox) {
                LOG_CRITICAL("Skybox initialization failed! Skybox pointer is NULL.");
            }

            m_debugDraw = std::make_unique<DebugDraw>();
            m_debugDraw->init();

            // Create the modular shadow system.
            m_shadowManager = std::make_unique<ShadowManager>();
            m_shadowManager->setQuality(ShadowQuality::High);
            LOG_INFO("ShadowManager created with High quality.");
        }

        void Renderer::render(float deltaTime) {
            if (!m_sceneManager || !m_lightingShader) return;

            auto* currentScene = m_sceneManager->GetCurrentScene();
            if (!currentScene) return;

            // Находим главную камеру для всего кадра
            auto* mainCamera = GetMainCamera(m_sceneManager);


            
            // =========================================================================
            // PASS 1: РЕНДЕРИНГ ТЕНЕЙ (Shadow Map Pass)
            // =========================================================================
            if (m_shadowManager) {
                glm::mat4 viewProj = glm::mat4(1.0f);
                float nearPlane = 0.1f;
                float farPlane = 400.0f;

                if (mainCamera) {
                    glm::mat4 viewMatrix = mainCamera->getViewMatrix();
                    m_lightingShader->setMat4("viewMatrix", viewMatrix);
                }

                m_shadowManager->renderShadows(currentScene, viewProj, nearPlane, farPlane);
            }
            
            // =========================================================================
            // PASS 2: ОСНОВНОЙ РЕНДЕРИНГ СЦЕНЫ (Forward Pass)
            // =========================================================================
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            glViewport(0, 0, Globals::screenWidth, Globals::screenHeight);

            glEnable(GL_DEPTH_TEST);
            glDepthMask(GL_TRUE);
            glDepthFunc(GL_LESS);
            glEnable(GL_CULL_FACE);
            glCullFace(GL_BACK);
            glFrontFace(GL_CCW);
            glDisable(GL_BLEND);

            glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            if (m_shadowManager) {
                m_lightingShader->use();
                m_shadowManager->bindShadowTextures(*m_lightingShader, currentScene);
            }

            // Вызываем рендер сцены (возвращает void)
            m_sceneManager->Render(*m_lightingShader, deltaTime);

            Globals::debugMode = m_debugOverlay ? m_debugOverlay->isVisible() : false;

            // Обновляем позицию в DebugOverlay

            // =========================================================================
            // PASS 3: РЕНДЕР SKYBOX
            // =========================================================================
            if (mainCamera && m_skybox && m_skyboxShader) {
                glm::mat4 view = mainCamera->getViewMatrix();
                glm::mat4 skyboxView = glm::mat4(glm::mat3(view)); // Убираем трансляцию, оставляя только вращение

                // 1. Плавное вращение вокруг оси Y (эффект плывущих облаков/ветра)
                float time = static_cast<float>(glfwGetTime()) * 0.015f;
                skyboxView = glm::rotate(skyboxView, time, glm::vec3(0.0f, 1.0f, 0.0f));

                glm::mat4 projection = mainCamera->getProjectionMatrix();

                glDepthFunc(GL_LEQUAL);
                glDepthMask(GL_FALSE);
                glDisable(GL_CULL_FACE);

                m_skyboxShader->use();
                m_skyboxShader->setMat4("view", skyboxView);
                m_skyboxShader->setMat4("projection", projection);

                // 2. Передаем время для анимаций во фрагментный шейдер
                m_skyboxShader->setFloat("u_time", static_cast<float>(glfwGetTime()));

                // 3. Дополнительно передаем позицию камеры (если в шейдере захочется сделать микро-параллакс)
                if (auto* camObj = mainCamera->owner) {
                    m_skyboxShader->setVec3("u_cameraPos", camObj->getWorldPosition());
                }

                m_skybox->Draw(*m_skyboxShader);

                glDepthMask(GL_TRUE);
                glEnable(GL_CULL_FACE);
                glDepthFunc(GL_LESS);
            }

            // =========================================================================
            // PASS 4: UI И DEBUG
            // =========================================================================
            if (m_uiManager) {
                glDisable(GL_DEPTH_TEST);
                glDisable(GL_CULL_FACE);
                glEnable(GL_BLEND);
                glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

                m_uiManager->render();
            }

            if (m_debugDraw && Globals::renderPhysicsDebug) {
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
                    int boxEdges[24] = { 0,1, 1,2, 2,3, 3,0, 4,5, 5,6, 6,7, 7,4, 0,4, 1,5, 2,6, 3,7 };

                    for (auto* collider : colliders) {
                        if (!collider || !collider->owner) continue;

                        glm::mat4 world = collider->owner->getWorldMatrix();
                        glm::vec3 pos = collider->GetWorldPosition();
                        glm::vec3 xAxis = glm::vec3(world[0]) * 0.5f;
                        glm::vec3 yAxis = glm::vec3(world[1]) * 0.5f;
                        glm::vec3 zAxis = glm::vec3(world[2]) * 0.5f;

                        m_debugDraw->DrawLine(pos, pos + xAxis, glm::vec3(1.0f, 0.0f, 0.0f));
                        m_debugDraw->DrawLine(pos, pos + yAxis, glm::vec3(0.0f, 1.0f, 0.0f));
                        m_debugDraw->DrawLine(pos, pos + zAxis, glm::vec3(0.0f, 0.0f, 1.0f));

                        if (auto* mesh = dynamic_cast<Lindo::Components::Physics::MeshCollider*>(collider)) {
                            const glm::vec3 halfSize = mesh->GetSize() * 0.5f;
                            glm::vec3 corners[8] = {
                                {-halfSize.x, -halfSize.y, -halfSize.z}, {halfSize.x, -halfSize.y, -halfSize.z},
                                {halfSize.x, halfSize.y, -halfSize.z}, {-halfSize.x, halfSize.y, -halfSize.z},
                                {-halfSize.x, -halfSize.y, halfSize.z}, {halfSize.x, -halfSize.y, halfSize.z},
                                {halfSize.x, halfSize.y, halfSize.z}, {-halfSize.x, halfSize.y, halfSize.z}
                            };

                            glm::vec3 worldCorners[8];
                            for (int i = 0; i < 8; ++i) {
                                worldCorners[i] = glm::vec3(world * glm::vec4(corners[i], 1.0f));
                            }
                            for (int idx = 0; idx < 24; idx += 2) {
                                m_debugDraw->DrawLine(worldCorners[boxEdges[idx]], worldCorners[boxEdges[idx + 1]], glm::vec3(1.0f, 0.0f, 1.0f));
                            }
                        }
                        else if (auto* box = dynamic_cast<Lindo::Components::Physics::BoxCollider*>(collider)) {
                            const glm::vec3 halfSize = box->GetSize() * 0.5f;
                            glm::vec3 corners[8] = {
                                {-halfSize.x, -halfSize.y, -halfSize.z}, {halfSize.x, -halfSize.y, -halfSize.z},
                                {halfSize.x, halfSize.y, -halfSize.z}, {-halfSize.x, halfSize.y, -halfSize.z},
                                {-halfSize.x, -halfSize.y, halfSize.z}, {halfSize.x, -halfSize.y, halfSize.z},
                                {halfSize.x, halfSize.y, halfSize.z}, {-halfSize.x, halfSize.y, halfSize.z}
                            };

                            glm::vec3 worldCorners[8];
                            for (int i = 0; i < 8; ++i) {
                                worldCorners[i] = glm::vec3(world * glm::vec4(corners[i], 1.0f));
                            }
                            for (int idx = 0; idx < 24; idx += 2) {
                                m_debugDraw->DrawLine(worldCorners[boxEdges[idx]], worldCorners[boxEdges[idx + 1]], glm::vec3(0.0f, 1.0f, 1.0f));
                            }
                        }
                        else if (auto* sphere = dynamic_cast<Lindo::Components::Physics::SphereCollider*>(collider)) {
                            float radius = sphere->GetRadius();
                            glm::vec3 worldCenter = pos;
                            float maxScale = glm::max(glm::max(glm::length(glm::vec3(world[0])), glm::length(glm::vec3(world[1]))), glm::length(glm::vec3(world[2])));
                            float worldRadius = radius * maxScale;

                            m_debugDraw->DrawSphere(worldCenter, worldRadius, glm::vec3(0.0f, 1.0f, 1.0f), 24);
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

            if (m_debugOverlay && m_debugOverlay->isVisible()) {
                if (m_uiManager && m_uiManager->getRenderer()) {
                    m_debugOverlay->render(*m_uiManager->getRenderer());
                }
            }
        }

        void Renderer::onResize(int width, int height) {
            Globals::screenWidth = width;
            Globals::screenHeight = height;
            if (height == 0) height = 1;

            auto* mainCamera = GetMainCamera(m_sceneManager);
            if (mainCamera) {
                mainCamera->setAspectRatio(static_cast<float>(width) / static_cast<float>(height));
            }

            glViewport(0, 0, width, height);
        }
    }
}