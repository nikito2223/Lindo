#pragma once

#include "Collider.h"
#include <Physics/Math/AABB.h>
#include <algorithm>

namespace Lindo {
    namespace Components {
        namespace Physics {

            // A sphere collider. The radius is the local-space radius;
            // the world-space radius is scaled by the gameObject's transform scale.
            // A local center offset is supported via the base Collider::offset.
            class SphereCollider : public Collider {
            private:
                float radius = 0.5f;

            public:
                SphereCollider() = default;
                explicit SphereCollider(float radius) : radius(std::max(0.0001f, radius)) {}
                void FitToAABB(const Lindo::Math::AABB& aabb);
                // ----- Configuration -----
                void SetRadius(float newRadius) { radius = std::max(0.0001f, newRadius); }
                float GetRadius() const { return radius; }

                // World-space radius (scaled by the gameObject's scale).
                float GetWorldRadius() const;

                // World-space center of the sphere.
                glm::vec3 GetWorldCenter() const override;

                // World-space AABB.
                Lindo::Math::AABB GetAABB() const override;

                // ----- Collision queries -----
                bool CheckCollision(Collider* other, CollisionInfo& outInfo) const override;
                bool CheckCollision(const BoxCollider* other, CollisionInfo& outInfo) const override;
                bool CheckCollision(const SphereCollider* other, CollisionInfo& outInfo) const override;
                bool CheckCollision(const CapsuleCollider* other, CollisionInfo& outInfo) const override;
                bool CheckCollision(const MeshCollider* other, CollisionInfo& outInfo) const override;

                // ----- Debug rendering -----
                void OnDrawGizmos() override;

            private:
                // Closest point on an oriented box to the sphere center.
                glm::vec3 ClosestPointOnBox(const BoxCollider* box, const glm::vec3& sphereCenter) const;
            };

        }
    }
}
