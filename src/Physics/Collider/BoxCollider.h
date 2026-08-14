#pragma once

#include "Collider.h"
#include <Physics/Math/AABB.h>

namespace Lindo {
    namespace Components {
        namespace Physics {

            // An oriented box collider. The "size" is the full local-space
            // extents (width, height, depth). The box follows the owner's
            // transform (position, rotation, scale). A local center offset is
            // supported via the base Collider::offset.
            class BoxCollider : public Collider {
            private:
                glm::vec3 size = glm::vec3(1.0f);

            public:
                BoxCollider() = default;
                explicit BoxCollider(const glm::vec3& size) : size(size) {}

                // ----- Configuration -----
                void SetSize(const glm::vec3& newSize) { size = glm::max(newSize, glm::vec3(0.0001f)); }
                glm::vec3 GetSize() const { return size; }

                // ----- World-space helpers -----
                // World-space half-extents (respecting owner + collider scale).
                glm::vec3 GetWorldHalfExtents() const;
                // World-space rotation as a 3x3 rotation matrix (from owner).
                glm::mat3 GetWorldRotationMatrix() const;

                // World-space AABB (axis-aligned bounds of the rotated box).
                void GetAABB(glm::vec3& outMin, glm::vec3& outMax) const;
                Lindo::Math::AABB GetAABB() const override;

                // The 8 world-space corners of the box.
                std::vector<glm::vec3> GetWorldVertices() const;

                // ----- Collision queries -----
                bool CheckCollision(Collider* other, CollisionInfo& outInfo) const override;
                bool CheckCollision(const BoxCollider* other, CollisionInfo& outInfo) const override;
                bool CheckCollision(const SphereCollider* other, CollisionInfo& outInfo) const override;
                bool CheckCollision(const CapsuleCollider* other, CollisionInfo& outInfo) const override;

                // ----- Debug rendering -----
                void OnDrawGizmos() override;

            private:
                // Transform a local-space point into world space using the
                // owner's rotation + position + offset.
                glm::vec3 TransformPoint(const glm::vec3& localPoint) const;
            };

        }
    }
}
