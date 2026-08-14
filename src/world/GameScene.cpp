#include "GameScene.h"
#include "Component/GameObject/GameObject.h"
#include "Component/PlayerController/Player.h"
#include "Component/Camera/Camera.h"
#include "Component/Physhcs/MeshRenderer.h"
#include "Component/Physhcs/RigidBody.h"
#include "Graphics/core/Model.h"
#include "core/Globals.h"
#include "Core/Input.h"
#include <Physics/Collider/BoxCollider.h>
#include <Physics/Collider/SphereCollider.h>
#include <Physics/Collider/CapsuleCollider.h>
#include <Physics/Collider/MeshCollider.h>
#include <memory>
#include <fstream>
#include <Component/Graphics/Light.h>
#include "Debug/DebugLogger.h"
#include <Core/AssetManager.h>

namespace Lindo {
    namespace Scenes {

        static std::shared_ptr<Lindo::Graphics::Model> s_cubeModel = nullptr;
        static std::shared_ptr<Lindo::Graphics::Model> s_planeModel = nullptr;
        static std::shared_ptr<Lindo::Graphics::Model> s_sphereModel = nullptr;
        static std::shared_ptr<Lindo::Graphics::Model> s_capsuleModel = nullptr;

        static bool fileExists(const std::string& path) {
            std::ifstream f(path);
            return f.good();
        }

        void GameScene::OnCreate() {
            LOG_INFO("==================================================");
            LOG_INFO("GameScene: Executing OnCreate()...");
            LOG_INFO("==================================================");

            std::string cubePath = "res/models/Defaults/cube.obj";
            std::string planePath = "res/models/Defaults/plane.obj";
            std::string spherePath = "res/models/Defaults/sphere.obj";
            std::string capsulePath = "res/models/Defaults/capsule.obj";

            try {
                LOG_INFO("Loading base models...");

                if (fileExists(cubePath)) {
                    s_cubeModel = std::make_shared<Lindo::Graphics::Model>(cubePath);
                    LOG_INFO("Cube model loaded");
                } else {
                    LOG_WARN("Cube model not found: " + cubePath);
                }

                if (fileExists(planePath)) {
                    s_planeModel = std::make_shared<Lindo::Graphics::Model>(planePath);
                    LOG_INFO("Plane model loaded");
                } else {
                    LOG_WARN("Plane model not found: " + planePath);
                }

                if (fileExists(spherePath)) {
                    s_sphereModel = std::make_shared<Lindo::Graphics::Model>(spherePath);
                    LOG_INFO("Sphere model loaded");
                } else {
                    LOG_WARN("Sphere model not found: " + spherePath);
                }

                if (fileExists(capsulePath)) {
                    s_capsuleModel = std::make_shared<Lindo::Graphics::Model>(capsulePath);
                    LOG_INFO("Capsule model loaded");
                } else {
                    LOG_WARN("Capsule model not found: " + capsulePath);
                }
            }
            catch (const std::exception& e) {
                LOG_ERROR("Failed to load models: " + std::string(e.what()));
            }

            CreatePlayer();
            CreateEnvironment();
            SetupLighting();

            LOG_INFO("GameScene initialized. GameObjects: " + std::to_string(gameObjects.size()));
            LOG_INFO("==================================================");
        }

        void GameScene::OnActivate() {
            LOG_INFO("GameScene activated");
        }

        void GameScene::Update(float deltaTime) {
            for (auto& obj : gameObjects) {
                if (obj) {
                    obj->Update(deltaTime);
                }
            }
        }

        void GameScene::Render(Graphics::Shader& shader, float deltaTime) {
            shader.use();

            shader.setInt("activePointLights", 0);
            shader.setBool("pointShadowsEnabled", false);

            // 1. Камера
            if (playerObject) {
                auto* camera = playerObject->getComponent<Lindo::Components::Rendering::Camera>();
                if (camera) {
                    shader.setMat4("view", camera->getViewMatrix());
                    shader.setMat4("projection", camera->getProjectionMatrix());
                    shader.setVec3("viewPos", playerObject->transform.position);
                    shader.setBool("showLightIcons", Globals::debugMode);
                    shader.setFloat("lightIconRadius", 1.5f);
                }
            }

            // 2. Направляющий свет
            auto* sunObject = FindGameObject("Sun");
            if (sunObject) {
                auto* sunLight = sunObject->getComponent<Lindo::Components::Light::DirectionalLight>();
                if (sunLight && sunLight->enabled) {
                    sunLight->ApplyToShader(shader, "dirLight");
                } else {
                    shader.setBool("dirLight.enabled", false);
                }
            } else {
                shader.setBool("dirLight.enabled", false);
            }

            // 3. Точечные источники света
            constexpr int MAX_POINT_LIGHTS = 32;
            auto pointLights = FindComponentsOfType<Lindo::Components::Light::PointLight>();
            int activeLights = 0;

            int shadowableIndex = 0;
            for (size_t i = 0; i < pointLights.size() && i < MAX_POINT_LIGHTS; ++i) {
                auto* pointLight = pointLights[i];
                if (!pointLight || !pointLight->enabled) continue;

                std::string uniformName = "pointLights[" + std::to_string(activeLights) + "]";
                pointLight->ApplyToShader(shader, uniformName);

                int shadowIndex = -1;
                if (pointLight->castShadows && shadowableIndex < 4) {
                    shadowIndex = shadowableIndex;
                    shadowableIndex++;
                }
                shader.setInt("u_pointShadowIndex[" + std::to_string(activeLights) + "]", shadowIndex);
                activeLights++;
            }

            for (int i = activeLights; i < MAX_POINT_LIGHTS; ++i) {
                shader.setInt("u_pointShadowIndex[" + std::to_string(i) + "]", -1);
            }

            shader.setInt("activePointLights", activeLights);

            // 4. Прожекторы
            auto* spotLight = FindComponentOfType<Lindo::Components::Light::SpotLight>();
            if (spotLight && spotLight->enabled) {
                spotLight->ApplyToShader(shader, "spotLight");
            } else {
                shader.setBool("spotLight.enabled", false);
            }

            // 5. Отрисовка объектов
            for (auto& obj : gameObjects) {
                if (!obj) continue;
                auto* renderer = obj->getComponent<Lindo::Components::Physics::MeshRenderer>();
                if (renderer && renderer->IsEnabled()) {
                    renderer->OnDraw(shader);
                }
            }
        }

        void GameScene::ProcessInput(Input::Input* input, float deltaTime) {
            if (!input || !playerObject) return;

            auto* player = playerObject->getComponent<Lindo::Components::Controller::Player>();
            if (!player) return;

            float forward = 0.0f;
            float right = 0.0f;

            if (input->isKeyPressed(GLFW_KEY_W)) forward += 1.0f;
            if (input->isKeyPressed(GLFW_KEY_S)) forward -= 1.0f;
            if (input->isKeyPressed(GLFW_KEY_D)) right += 1.0f;
            if (input->isKeyPressed(GLFW_KEY_A)) right -= 1.0f;

            player->MoveForward(forward);
            player->MoveRight(right);

            if (input->isKeyPressed(GLFW_KEY_SPACE)) {
                player->Jump();
            }

            if (input->isKeyPressed(GLFW_KEY_LEFT_SHIFT)) {
                player->StartRunning();
            } else {
                player->StopRunning();
            }

            glm::vec2 mouseDelta = input->getMouseDelta();
            if (mouseDelta.x != 0.0f || mouseDelta.y != 0.0f) {
                player->Look(mouseDelta.x, mouseDelta.y);
                input->resetMouseDelta();
            }
        }

        void GameScene::OnDestroy() {
            LOG_INFO("==================================================");
            LOG_INFO("GameScene: Destroying...");
            LOG_INFO("==================================================");
            
            playerObject = nullptr;
            cameraObject = nullptr;
            s_cubeModel = nullptr;
            s_planeModel = nullptr;
            s_sphereModel = nullptr;
            s_capsuleModel = nullptr;
            
            LOG_INFO("GameScene destroyed");
            LOG_INFO("==================================================");
        }

        void GameScene::CreatePlayer() {
            LOG_INFO("Creating Player...");
            
            playerObject = CreateGameObject("Player");
            playerObject->transform.position = glm::vec3(0.0f, 2.0f, 0.0f);

            playerObject->addComponent<Lindo::Components::Controller::Player>();
            playerObject->addComponent<Lindo::Components::Rendering::Camera>();
            
            LOG_INFO("Player created at (0, 2, 0)");
        }

        void GameScene::CreateEnvironment() {
            LOG_INFO("Creating environment...");

            // === 1. Земля ===
            auto* ground = CreateGameObject("Ground");
            ground->transform.position = glm::vec3(0.0f, 0.0f, 0.0f);
            ground->transform.scale = glm::vec3(20.0f, 1.0f, 20.0f);

            auto* groundRenderer = ground->addComponent<Lindo::Components::Physics::MeshRenderer>();
            groundRenderer->setModel(s_planeModel ? s_planeModel.get() : s_cubeModel.get());

            auto* groundCollider = ground->addComponent<Lindo::Components::Physics::MeshCollider>();
            if (groundCollider) {
                groundCollider->UpdateFromMeshRenderer();
            }

            // === 2. Сфера - Красная ===
            auto* sphere = CreateGameObject("Sphere_Prop");
            sphere->transform.position = glm::vec3(-6.0f, 1.0f, 5.0f);
            sphere->transform.scale = glm::vec3(1.0f, 1.0f, 1.0f);
            sphere->castsShadows = true;

            auto* sphereRenderer = sphere->addComponent<Lindo::Components::Physics::MeshRenderer>();
            sphereRenderer->setModel(s_sphereModel ? s_sphereModel.get() : s_cubeModel.get());

            auto* sphereMat = sphere->addComponent<Lindo::Graphics::Material>(0, 0, 32.0f, false);
            sphereMat->useTexture = false;
            sphereMat->color = glm::vec3(0.9f, 0.2f, 0.2f);
            sphereRenderer->material = sphereMat;

            auto* sphereCollider = sphere->addComponent<Lindo::Components::Physics::SphereCollider>();
            if (sphereCollider) {
                sphereCollider->SetRadius(sphere->transform.scale.x);
            }

            // === 3. Капсула - Зеленая ===
            auto* capsule = CreateGameObject("Capsule_Prop");
            capsule->transform.position = glm::vec3(-2.0f, 1.0f, 5.0f);
            capsule->transform.scale = glm::vec3(1.0f, 1.0f, 1.0f);

            auto* capsuleRenderer = capsule->addComponent<Lindo::Components::Physics::MeshRenderer>();
            capsuleRenderer->setModel(s_capsuleModel ? s_capsuleModel.get() : s_cubeModel.get());

            auto* capsuleMat = capsule->addComponent<Lindo::Graphics::Material>(0, 0, 32.0f, false);
            capsuleMat->useTexture = false;
            capsuleMat->color = glm::vec3(0.2f, 0.8f, 0.2f);
            capsuleRenderer->material = capsuleMat;

            auto* capsuleCollider = capsule->addComponent<Lindo::Components::Physics::CapsuleCollider>();
            if (capsuleCollider) {
                capsuleCollider->SetDirection(Lindo::Components::Physics::CapsuleCollider::Direction::Y);
                if (s_capsuleModel) {
                    capsuleCollider->FitToAABB(s_capsuleModel->getAABB());
                }
            }

            // === 4. Куб - Синий ===
            auto* cube = CreateGameObject("Cube_Prop");
            cube->transform.position = glm::vec3(2.0f, 4.0f, 5.0f);
            cube->transform.scale = glm::vec3(1.0f, 1.0f, 1.0f);

            auto* cubeRenderer = cube->addComponent<Lindo::Components::Physics::MeshRenderer>();
            cubeRenderer->setModel(s_cubeModel ? s_cubeModel.get() : nullptr);

            auto* cubeMat = cube->addComponent<Lindo::Graphics::Material>(0, 0, 32.0f, false);
            cubeMat->useTexture = false;
            cubeMat->color = glm::vec3(0.2f, 0.4f, 0.9f);
            cubeRenderer->material = cubeMat;

            auto* boxCollider = cube->addComponent<Lindo::Components::Physics::BoxCollider>();
            if (boxCollider) {
                boxCollider->SetSize(cube->transform.scale * 2.0f);
            }

            // === 5. Наклонная платформа - Желтая ===
            auto* ramp = CreateGameObject("Ramp_Platform");
            ramp->transform.position = glm::vec3(6.0f, 1.0f, 5.0f);
            ramp->transform.rotation = glm::vec3(0.0f, 0.0f, 45.0f);
            ramp->transform.scale = glm::vec3(2.0f, 1.0f, 2.0f);

            auto* rampRenderer = ramp->addComponent<Lindo::Components::Physics::MeshRenderer>();
            rampRenderer->setModel(s_planeModel ? s_planeModel.get() : s_cubeModel.get());

            auto* rampMat = ramp->addComponent<Lindo::Graphics::Material>(0, 0, 32.0f, false);
            rampMat->useTexture = false;
            rampMat->color = glm::vec3(0.9f, 0.8f, 0.2f);
            rampRenderer->material = rampMat;

            auto* rampCollider = ramp->addComponent<Lindo::Components::Physics::MeshCollider>();
            if (rampCollider) {
                rampCollider->UpdateFromMeshRenderer();
            }

            LOG_INFO("Environment created (5 objects)");
        }

        void GameScene::SetupLighting() {
            LOG_INFO("Setting up lighting...");
            
            auto* sunObject = CreateGameObject("Sun");
            sunObject->transform.position = glm::vec3(0.0f, 20.0f, 10.0f);
            sunObject->transform.rotation = glm::vec3(0.0f, -90.0f, 0.0f);

            auto* sunLight = sunObject->addComponent<Lindo::Components::Light::DirectionalLight>("SunLight");
            sunLight->direction = glm::normalize(glm::vec3(-0.5f, -1.0f, -0.5f));
            sunLight->SetBaseColor(glm::vec3(1.0f, 0.95f, 0.8f));
            sunLight->intensity = 1.0f;
            sunLight->ambient = glm::vec3(0.2f, 0.2f, 0.25f);
            sunLight->diffuse = glm::vec3(0.9f, 0.85f, 0.75f);
            sunLight->specular = glm::vec3(1.0f, 1.0f, 1.0f);
            sunLight->castShadows = true;
            
            LOG_INFO("Lighting setup complete");
        }

    }
}