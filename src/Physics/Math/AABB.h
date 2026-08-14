#pragma once

#include <glm/glm.hpp>
#include <limits>
#include <algorithm>

namespace Lindo {
    namespace Components {
        namespace Physics {
            class BoxCollider;
            class SphereCollider;
            class CapsuleCollider;
            class Collider;
        }
    }
}

namespace Lindo {
    namespace Math {

        // Axis-Aligned Bounding Box.
        // Used as the broad-phase test volume for every collider type.
        class AABB {
        public:
            glm::vec3 min;
            glm::vec3 max;

            AABB();
            AABB(const AABB& other) = default;
            AABB& operator=(const AABB& other) = default;
            AABB(const glm::vec3& minPoint, const glm::vec3& maxPoint);

            static AABB fromCenterSize(const glm::vec3& center, const glm::vec3& halfSize);

            void expand(const glm::vec3& point);
            void merge(const AABB& other);
            void invalidate();
            bool isValid() const;

            glm::vec3 getCenter() const;
            glm::vec3 getHalfSize() const;
            glm::vec3 getSize() const;

            float distanceToPoint(const glm::vec3& point) const;
            float squaredDistanceToPoint(const glm::vec3& point) const;

            // ----- Intersection tests (broad-phase) -----
            bool intersectRay(const glm::vec3& origin, const glm::vec3& dir,
                float& tmin, float& tmax) const;
            bool intersectAABB(const AABB& other) const;
            bool intersectSphere(const glm::vec3& center, float radius) const;
            bool intersectCapsule(const glm::vec3& lineA, const glm::vec3& lineB, float radius) const;

            // ----- Intersections with colliders (narrow-phase via their AABB) -----
            bool intersects(const Lindo::Components::Physics::BoxCollider* box) const;
            bool intersects(const Lindo::Components::Physics::SphereCollider* sphere) const;
            bool intersects(const Lindo::Components::Physics::CapsuleCollider* capsule) const;
            bool intersects(const Lindo::Components::Physics::Collider* collider) const;

            // Distance between a segment and this AABB.
            // Returns the closest point on the segment to the box.
            static float distanceSegmentAABB(const glm::vec3& a, const glm::vec3& b,
                const glm::vec3& min, const glm::vec3& max,
                glm::vec3& closestOnSegment);
        };

    }
}
