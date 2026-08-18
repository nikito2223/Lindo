#include "AABB.h"
#include "VectorMath.h"
#include <Physics/Collider/BoxCollider.h>
#include <Physics/Collider/SphereCollider.h>
#include <Physics/Collider/CapsuleCollider.h>
#include <Physics/Collider/Collider.h>

namespace Lindo {
    namespace Math {

        AABB::AABB() {
            invalidate();
        }

        AABB::AABB(const glm::vec3& minPoint, const glm::vec3& maxPoint)
            : min(minPoint), max(maxPoint) {}

        AABB AABB::fromCenterSize(const glm::vec3& center, const glm::vec3& halfSize) {
            return AABB(center - halfSize, center + halfSize);
        }

        void AABB::expand(const glm::vec3& point) {
            min = glm::min(min, point);
            max = glm::max(max, point);
        }

        void AABB::merge(const AABB& other) {
            if (!other.isValid()) return;
            if (!isValid()) {
                min = other.min;
                max = other.max;
                return;
            }
            min = glm::min(min, other.min);
            max = glm::max(max, other.max);
        }

        void AABB::invalidate() {
            min = glm::vec3(std::numeric_limits<float>::infinity());
            max = glm::vec3(-std::numeric_limits<float>::infinity());
        }

        bool AABB::isValid() const {
            return min.x <= max.x && min.y <= max.y && min.z <= max.z;
        }

        glm::vec3 AABB::getCenter() const {
            return (min + max) * 0.5f;
        }

        glm::vec3 AABB::getHalfSize() const {
            return (max - min) * 0.5f;
        }

        glm::vec3 AABB::getSize() const {
            return max - min;
        }

        float AABB::squaredDistanceToPoint(const glm::vec3& point) const {
            glm::vec3 closest = VectorMath::ClosestPointOnAABB(min, max, point);
            glm::vec3 diff = point - closest;
            return glm::dot(diff, diff);
        }

        float AABB::distanceToPoint(const glm::vec3& point) const {
            return std::sqrt(squaredDistanceToPoint(point));
        }

        bool AABB::intersectRay(const glm::vec3& origin, const glm::vec3& dir,
            float& tmin, float& tmax) const {
            glm::vec3 invDir(1.0f);
            for (int i = 0; i < 3; ++i) {
                if (std::fabs(dir[i]) > 1e-9f) {
                    invDir[i] = 1.0f / dir[i];
                }
            }

            float t0 = (min.x - origin.x) * invDir.x;
            float t1 = (max.x - origin.x) * invDir.x;
            if (invDir.x < 0.0f) std::swap(t0, t1);

            float t2 = (min.y - origin.y) * invDir.y;
            float t3 = (max.y - origin.y) * invDir.y;
            if (invDir.y < 0.0f) std::swap(t2, t3);

            float t4 = (min.z - origin.z) * invDir.z;
            float t5 = (max.z - origin.z) * invDir.z;
            if (invDir.z < 0.0f) std::swap(t4, t5);

            tmin = std::max(std::max(t0, t2), t4);
            tmax = std::min(std::min(t1, t3), t5);

            return tmax >= tmin && tmax >= 0.0f;
        }

        bool AABB::intersectAABB(const AABB& other) const {
            return min.x <= other.max.x && max.x >= other.min.x &&
                   min.y <= other.max.y && max.y >= other.min.y &&
                   min.z <= other.max.z && max.z >= other.min.z;
        }

        bool AABB::intersectSphere(const glm::vec3& center, float radius) const {
            float distSq = squaredDistanceToPoint(center);
            return distSq <= radius * radius;
        }

        bool AABB::intersectCapsule(const glm::vec3& lineA, const glm::vec3& lineB, float radius) const {
            glm::vec3 closest;
            float dist = distanceSegmentAABB(lineA, lineB, min, max, closest);
            return dist <= radius;
        }

        bool AABB::intersects(const Lindo::Components::Physics::BoxCollider* box) const {
            if (!box) return false;
            glm::vec3 bmin, bmax;
            box->GetAABB();
            return intersectAABB(AABB(bmin, bmax));
        }

        bool AABB::intersects(const Lindo::Components::Physics::SphereCollider* sphere) const {
            if (!sphere) return false;
            return intersectSphere(sphere->GetWorldCenter(), sphere->GetRadius());
        }

        bool AABB::intersects(const Lindo::Components::Physics::CapsuleCollider* capsule) const {
            if (!capsule) return false;
            glm::vec3 top, bottom;
            capsule->GetEndpoints(bottom, top);
            return intersectCapsule(bottom, top, capsule->GetRadius());
        }

        bool AABB::intersects(const Lindo::Components::Physics::Collider* collider) const {
            if (!collider) return false;
            return collider->GetAABB().intersectAABB(*this);
        }

        float AABB::distanceSegmentAABB(const glm::vec3& a, const glm::vec3& b,
            const glm::vec3& min, const glm::vec3& max,
            glm::vec3& closestOnSegment) {
            // If either endpoint is inside the box, the distance is zero.
            glm::vec3 clampedA = VectorMath::ClosestPointOnAABB(min, max, a);
            glm::vec3 clampedB = VectorMath::ClosestPointOnAABB(min, max, b);

            bool aInside = glm::length(clampedA - a) < 1e-6f;
            bool bInside = glm::length(clampedB - b) < 1e-6f;

            if (aInside || bInside) {
                closestOnSegment = aInside ? a : b;
                return 0.0f;
            }

            // Sample points along the segment and keep the closest to the box.
            // The segment-capsule broad-phase test is robust enough with a
            // reasonably dense sample set; the returned point is conservative
            // (may be slightly off the true closest point but always valid).
            closestOnSegment = a;
            glm::vec3 toBox = clampedA - a;
            float bestDistSq = glm::dot(toBox, toBox);

            const int samples = 32;
            for (int i = 1; i <= samples; ++i) {
                float t = static_cast<float>(i) / static_cast<float>(samples);
                glm::vec3 p = glm::mix(a, b, t);
                glm::vec3 clamped = VectorMath::ClosestPointOnAABB(min, max, p);
                glm::vec3 diff = clamped - p;
                float distSq = glm::dot(diff, diff);
                if (distSq < bestDistSq) {
                    bestDistSq = distSq;
                    closestOnSegment = p;
                }
            }
            return std::sqrt(bestDistSq);
        }

    }
}
