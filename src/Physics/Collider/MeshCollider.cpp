#include "MeshCollider.h"
#include <Physics/PhysicsSystem.h>
#include <Component/GameObject/GameObject.h>

namespace Lindo {
    namespace Components {
        namespace Physics {

            void MeshCollider::OnStart() {
                UpdateFromMeshRenderer();
                Collider::OnStart();
            }

            void MeshCollider::OnDestroy() {
                Collider::OnDestroy();
            }

            void MeshCollider::SetLocalBounds(const glm::vec3& minBounds, const glm::vec3& maxBounds) {
                localMin = minBounds;
                localMax = maxBounds;
                UpdateBounds();
            }

            void MeshCollider::UpdateFromMeshRenderer() {
                if (!owner) return;

                auto* meshRenderer = owner->getComponent<Lindo::Components::Physics::MeshRenderer>();
                if (!meshRenderer || !meshRenderer->hasBBox) {
                    return;
                }

                localMin = meshRenderer->bboxMin;
                localMax = meshRenderer->bboxMax;
                UpdateBounds();
            }

            void MeshCollider::UpdateBounds() {
                glm::vec3 size = localMax - localMin;
                SetSize(glm::abs(size));

                glm::vec3 center = (localMax + localMin) * 0.5f;
                SetOffset(center);
            }

        }
    }
}
