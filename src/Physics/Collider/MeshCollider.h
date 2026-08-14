#pragma once

#include "BoxCollider.h"
#include <Component/Physhcs/MeshRenderer.h>

namespace Lindo {
    namespace Components {
        namespace Physics {

            class MeshCollider : public BoxCollider {
            public:
                MeshCollider() = default;
                virtual ~MeshCollider() = default;

                void OnStart() override;
                void OnDestroy() override;

                void SetLocalBounds(const glm::vec3& minBounds, const glm::vec3& maxBounds);
                void UpdateFromMeshRenderer();

            private:
                void UpdateBounds();
                glm::vec3 localMin{ -0.5f, -0.5f, -0.5f };
                glm::vec3 localMax{ 0.5f, 0.5f, 0.5f };
            };

        }
    }
}
