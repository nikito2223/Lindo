#include "GameScene.h"
#include "Component/GameObject/GameObject.h"
#include "Component/PlayerController/Player.h"
#include "Component/Camera/Camera.h"
#include "Component/Physhcs/MeshRenderer.h"
#include "Component/Physhcs/RigidBody.h"
#include "Graphics/core/Model.h"
#include "Core/Input.h"
#include <Physics/Collider/BoxCollider.h>
#include <Physics/Collider/SphereCollider.h>
#include <Physics/Collider/CapsuleCollider.h>
#include <Physics/Collider/MeshCollider.h>
#include "core/Types/Settings.h"
#include <memory>
#include <fstream>
#include <Component/Graphics/Light.h>
#include "Debug/DebugLogger.h"
#include <Core/AssetManager.h>
#include <filesystem>
#include <Core/Time/Time.h>


namespace Lindo {
    namespace Scenes {

        static std::shared_ptr<Lindo::Graphics::Model> s_cubeModel = nullptr;
        static std::shared_ptr<Lindo::Graphics::Model> s_planeModel = nullptr;
        static std::shared_ptr<Lindo::Graphics::Model> s_sphereModel = nullptr;
        static std::shared_ptr<Lindo::Graphics::Model> s_capsuleModel = nullptr;
        static std::shared_ptr<Lindo::Graphics::Model> s_CarobrfModel = nullptr;

        void GameScene::OnCreate() {
            LOG_INFO("==================================================");
            LOG_INFO("GameScene: Executing OnCreate()...");
            LOG_INFO("==================================================");

            auto& assets = Lindo::AssetManager::get();

            std::string cubePath = assets.resolvePath("cube.obj", "models");
            std::string planePath = assets.resolvePath("plane.obj", "models");
            std::string spherePath = assets.resolvePath("sphere.obj", "models");
            std::string capsulePath = assets.resolvePath("capsule.obj", "models");
            std::string CarobkaPath = assets.resolvePath("Contener.fbx", "models");

            try {
                LOG_INFO("Loading base models...");

                s_cubeModel = std::make_shared<Lindo::Graphics::Model>(cubePath);
                LOG_INFO("Model Cube - True");

                s_planeModel = std::make_shared<Lindo::Graphics::Model>(planePath);
                LOG_INFO("Model Plane - True");

                s_sphereModel = std::make_shared<Lindo::Graphics::Model>(spherePath);
                LOG_INFO("Model Sphere - True");

                s_capsuleModel = std::make_shared<Lindo::Graphics::Model>(capsulePath);
                s_CarobrfModel = std::make_shared<Lindo::Graphics::Model>(CarobkaPath);
                LOG_INFO("Model Capsule - True");
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

        void GameScene::Update() {
            float dt = Lindo::Time::GetDeltaTime();
            for (auto& obj : gameObjects) {
                if (obj) {
                    obj->Update();

                    // === ДИНАМИКА: Вращение объектов по оси ===
                    if (obj->name == "Rotating_Platform") {
                        obj->transform.rotation.y += 30.0f * dt; // Медленное вращение платформы
                    }
                    else if (obj->name == "Spinning_Blade") {
                        obj->transform.rotation.z += 90.0f * dt; // Быстрое вращение лопасти/препятствия
                    }
                }
            }
        }

        void GameScene::Render(Graphics::Shader& shader) {
            shader.use();

            shader.setInt("activePointLights", 0);
            shader.setBool("pointShadowsEnabled", false);

            Settings& settings = Settings::getInstance();

            // 1. Камера
            if (playerObject) {
                auto* camera = playerObject->getComponent<Lindo::Components::Rendering::Camera>();
                if (camera) {
                    shader.setMat4("view", camera->getViewMatrix());
                    shader.setMat4("projection", camera->getProjectionMatrix());
                    shader.setMat4("viewMatrix", camera->getViewMatrix());
                    shader.setVec3("viewPos", playerObject->transform.position);
                    shader.setBool("showLightIcons", settings.debugMode);
                    shader.setFloat("lightIconRadius", 1.5f);
                }
            }

            // 2. Направляющий свет (Солнце)
            auto* sunObject = FindGameObject("Sun");
            if (sunObject) {
                auto* sunLight = sunObject->getComponent<Lindo::Components::Light::DirectionalLight>();
                if (sunLight && sunLight->enabled) {
                    sunLight->ApplyToShader(shader, "dirLight");
                }
                else {
                    shader.setBool("dirLight.enabled", false);
                }
            }
            else {
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
            }
            else {
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

        void GameScene::ProcessInput(Input::Input* input) {
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
            }
            else {
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
            playerObject->transform.position = glm::vec3(0.0f, 2.0f, -5.0f);

            playerObject->addComponent<Lindo::Components::Controller::Player>();
            playerObject->addComponent<Lindo::Components::Rendering::Camera>();

            LOG_INFO("Player created at (0, 2, -5)");
        }

        void GameScene::CreateEnvironment() {
            LOG_INFO("Creating expanded test polygon environment...");

            // === 1. Главная земля (Основной пол) ===
            auto* ground = CreateGameObject("Ground");
            ground->transform.position = glm::vec3(0.0f, 0.0f, 0.0f);
            ground->transform.scale = glm::vec3(25.0f, 1.0f, 25.0f);

            auto* groundRenderer = ground->addComponent<Lindo::Components::Physics::MeshRenderer>();
            groundRenderer->setModel(s_planeModel ? s_planeModel.get() : s_cubeModel.get());

            auto* groundMat = ground->addComponent<Lindo::Graphics::Material>(0, 0, 32.0f, false);
            groundMat->color = glm::vec3(0.3f, 0.3f, 0.35f);
            groundRenderer->material = groundMat;

            auto* groundCollider = ground->addComponent<Lindo::Components::Physics::MeshCollider>();
            if (groundCollider) groundCollider->UpdateFromMeshRenderer();


            // === 2. Зона паркура (Ступени разной высоты для проверки прыжков) ===
            for (int i = 1; i <= 4; ++i) {
                auto* step = CreateGameObject("Stair_Step_" + std::to_string(i));
                step->transform.position = glm::vec3(0.0f, i * 0.5f, i * 3.0f);
                step->transform.scale = glm::vec3(3.0f, i * 0.5f, 2.0f);

                auto* stepRenderer = step->addComponent<Lindo::Components::Physics::MeshRenderer>();
                stepRenderer->setModel(s_cubeModel.get());

                auto* stepMat = step->addComponent<Lindo::Graphics::Material>(0, 0, 32.0f, false);
                stepMat->color = glm::vec3(0.2f, 0.6f + (i * 0.1f), 0.4f);
                stepRenderer->material = stepMat;

                auto* boxCol = step->addComponent<Lindo::Components::Physics::BoxCollider>();
                if (boxCol) boxCol->FitToAABB(s_cubeModel->getAABB());
            }


            auto* step_ = CreateGameObject("Stair_Step");
            step_->transform.position = glm::vec3(0.0f, 5.0f, -2.0f);
            step_->transform.scale = glm::vec3(1.0f, 1.0f, 1.0f);

            auto* stepRenderer = step_->addComponent<Lindo::Components::Physics::MeshRenderer>();
            stepRenderer->setModel(s_CarobrfModel.get());

            auto* stepMat = step_->addComponent<Lindo::Graphics::Material>(0, 0, 32.0f, false);
            stepMat->color = glm::vec3(0.2f, 0.6f, 0.4f);
            stepRenderer->material = stepMat;

            auto* boxCol = step_->addComponent<Lindo::Components::Physics::MeshCollider>();
            if (boxCol) boxCol->UpdateFromMeshRenderer();
            auto* RigCol = step_->addComponent<Lindo::Components::Physics::RigidBody>();

            // === 3. Динамическая зона: Вращающаяся платформа ===
            auto* rotPlatform = CreateGameObject("Rotating_Platform");
            rotPlatform->transform.position = glm::vec3(8.0f, 1.5f, 5.0f);
            rotPlatform->transform.scale = glm::vec3(4.0f, 0.3f, 4.0f);

            auto* rpRenderer = rotPlatform->addComponent<Lindo::Components::Physics::MeshRenderer>();
            rpRenderer->setModel(s_cubeModel.get());

            auto* rpMat = rotPlatform->addComponent<Lindo::Graphics::Material>(0, 0, 32.0f, false);
            rpMat->color = glm::vec3(0.8f, 0.4f, 0.1f); // Оранжевая крутящаяся платформа
            rpRenderer->material = rpMat;

            auto* rpCol = rotPlatform->addComponent<Lindo::Components::Physics::BoxCollider>();
            if (rpCol) rpCol->FitToAABB(s_cubeModel->getAABB());


            // === 4. Динамическая зона: Вращающаяся лопасть (препятствие) ===
            auto* blade = CreateGameObject("Spinning_Blade");
            blade->transform.position = glm::vec3(8.0f, 2.5f, 12.0f);
            blade->transform.scale = glm::vec3(5.0f, 0.4f, 0.8f);

            auto* bladeRend = blade->addComponent<Lindo::Components::Physics::MeshRenderer>();
            bladeRend->setModel(s_cubeModel.get());

            auto* bladeMat = blade->addComponent<Lindo::Graphics::Material>(0, 0, 32.0f, false);
            bladeMat->color = glm::vec3(0.9f, 0.1f, 0.1f); // Красная опасная лопасть
            bladeRend->material = bladeMat;

            auto* bladeCol = blade->addComponent<Lindo::Components::Physics::BoxCollider>();
            if (bladeCol) bladeCol->FitToAABB(s_cubeModel->getAABB());


            // === 5. Шоурум примитивов (Слева от спавна) ===
            // Красная сфера
            auto* sphere = CreateGameObject("Sphere_Prop");
            sphere->transform.position = glm::vec3(-6.0f, 1.0f, 5.0f);
            sphere->transform.scale = glm::vec3(1.0f, 1.0f, 1.0f);
            sphere->castsShadows = true;

            auto* sphereRenderer = sphere->addComponent<Lindo::Components::Physics::MeshRenderer>();
            sphereRenderer->setModel(s_sphereModel ? s_sphereModel.get() : s_cubeModel.get());

            auto* sphereMat = sphere->addComponent<Lindo::Graphics::Material>(0, 0, 32.0f, false);
            sphereMat->color = glm::vec3(0.9f, 0.2f, 0.2f);
            sphereRenderer->material = sphereMat;

            auto* sphereCollider = sphere->addComponent<Lindo::Components::Physics::SphereCollider>();
            if (sphereCollider) sphereCollider->FitToAABB(s_sphereModel->getAABB());


            // Зеленая капсула
            auto* capsule = CreateGameObject("Capsule_Prop");
            capsule->transform.position = glm::vec3(-6.0f, 1.0f, 8.0f);
            capsule->transform.scale = glm::vec3(1.0f, 1.0f, 1.0f);

            auto* capsuleRenderer = capsule->addComponent<Lindo::Components::Physics::MeshRenderer>();
            capsuleRenderer->setModel(s_capsuleModel ? s_capsuleModel.get() : s_cubeModel.get());

            auto* capsuleMat = capsule->addComponent<Lindo::Graphics::Material>(0, 0, 32.0f, false);
            capsuleMat->color = glm::vec3(0.2f, 0.8f, 0.2f);
            capsuleRenderer->material = capsuleMat;

            auto* capsuleCollider = capsule->addComponent<Lindo::Components::Physics::CapsuleCollider>();
            if (capsuleCollider) {
                capsuleCollider->SetDirection(Lindo::Components::Physics::CapsuleCollider::Direction::Y);
                if (s_capsuleModel) capsuleCollider->FitToAABB(s_capsuleModel->getAABB());
            }


            // Желтая наклонная рампа
            auto* ramp = CreateGameObject("Ramp_Platform");
            ramp->transform.position = glm::vec3(-6.0f, 1.0f, 11.0f);
            ramp->transform.rotation = glm::vec3(0.0f, 0.0f, 30.0f);
            ramp->transform.scale = glm::vec3(2.0f, 1.0f, 2.0f);

            auto* rampRenderer = ramp->addComponent<Lindo::Components::Physics::MeshRenderer>();
            rampRenderer->setModel(s_planeModel ? s_planeModel.get() : s_cubeModel.get());

            auto* rampMat = ramp->addComponent<Lindo::Graphics::Material>(0, 0, 32.0f, false);
            rampMat->color = glm::vec3(0.9f, 0.8f, 0.2f);
            rampRenderer->material = rampMat;

            auto* rampCollider = ramp->addComponent<Lindo::Components::Physics::MeshCollider>();
            if (rampCollider) rampCollider->UpdateFromMeshRenderer();

            LOG_INFO("Test polygon environment created successfully.");
        }

        void GameScene::SetupLighting() {
            LOG_INFO("Setting up advanced lighting...");

            // Главное солнце
            auto* sunObject = CreateGameObject("Sun");
            sunObject->transform.position = glm::vec3(0.0f, 25.0f, 10.0f);
            sunObject->transform.rotation = glm::vec3(0.0f, -90.0f, 0.0f);

            auto* sunLight = sunObject->addComponent<Lindo::Components::Light::DirectionalLight>("SunLight");
            sunLight->direction = glm::normalize(glm::vec3(-0.4f, -1.0f, -0.4f));
            sunLight->SetBaseColor(glm::vec3(1.0f, 0.95f, 0.85f));
            sunLight->intensity = 1.2f;
            sunLight->ambient = glm::vec3(0.15f, 0.15f, 0.2f);
            sunLight->diffuse = glm::vec3(0.9f, 0.85f, 0.75f);
            sunLight->specular = glm::vec3(1.0f, 1.0f, 1.0f);
            sunLight->castShadows = true;

            // Дополнительный цветной точечный источник около вращающейся платформы
            //auto* pointLightObj = CreateGameObject("Neon_PointLight");
            //pointLightObj->transform.position = glm::vec3(8.0f, 5.0f, 5.0f);

            //auto* pointLight = pointLightObj->addComponent<Lindo::Components::Light::PointLight>("PointLightTest");
            //pointLight->color = glm::vec3(1.0f, 0.8f, 1.0f); // Голубое неоновое свечение
            //pointLight->intensity = 1.0f;
            //pointLight->constant = 1.0f;
            //pointLight->linear = 0.09f;
            //pointLight->quadratic = 0.032f;
            //pointLight->castShadows = true;

            LOG_INFO("Advanced lighting setup complete.");
        }

    }
}