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
            void Update() override;
            void Render(Graphics::Shader& shader) override;
            void ProcessInput(Input::Input* input) override;
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