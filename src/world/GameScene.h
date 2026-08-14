#pragma once
#include "world/Scene.h"

namespace Lindo {
    namespace World {
        class GameObject;
    }
}

namespace Lindo {
    namespace Scenes {

        class GameScene : public World::Scene {
        public:
            void OnCreate() override;
            void OnActivate() override;
            void Update(float deltaTime) override;
            void Render(Graphics::Shader& shader, float deltaTime) override;
            void ProcessInput(Input::Input* input, float deltaTime) override;
            void OnDestroy() override;

        private:
            void CreatePlayer();
            void CreateEnvironment();
            void SetupLighting();

            Lindo::World::GameObject* playerObject = nullptr;
            Lindo::World::GameObject* cameraObject = nullptr;
        };

    }
}