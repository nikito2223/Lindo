#pragma once

#include <glm/glm.hpp>
#include <algorithm>
#include <limits>

namespace Lindo {
    namespace Math {

        // Utility math helpers used heavily by the collision routines.
        // All functions operate on world-space glm::vec3 values.
        class VectorMath {
        public:
            // Closest point on segment [a, b] to a given point p.
            // Also returns the normalized parameter t in [0,1] along the segment.
            static glm::vec3 ClosestPointOnSegment(
                const glm::vec3& a,
                const glm::vec3& b,
                const glm::vec3& p,
                float& outT);

            // Closest point on segment [a, b] to point p (parameter discarded).
            static glm::vec3 ClosestPointOnSegment(const glm::vec3& a, const glm::vec3& b, const glm::vec3& p);

            // Squared distance from point p to segment [a, b].
            static float SquaredDistancePointSegment(const glm::vec3& a, const glm::vec3& b, const glm::vec3& p);

            // Distance from point p to segment [a, b].
            static float DistancePointSegment(const glm::vec3& a, const glm::vec3& b, const glm::vec3& p);

            // Closest points between two segments [p1,p2] and [q1,q2].
            // Stores the closest point on each segment in outP and outQ.
            static void ClosestPointsBetweenSegments(
                const glm::vec3& p1, const glm::vec3& p2,
                const glm::vec3& q1, const glm::vec3& q2,
                glm::vec3& outP, glm::vec3& outQ);

            // Squared distance between two segments.
            static float SquaredDistanceSegments(
                const glm::vec3& p1, const glm::vec3& p2,
                const glm::vec3& q1, const glm::vec3& q2);

            // Closest point on an axis-aligned box [min, max] to a given point p.
            static glm::vec3 ClosestPointOnAABB(const glm::vec3& min, const glm::vec3& max, const glm::vec3& p);

            // Absolute component-wise value of a vec3.
            static glm::vec3 Abs(const glm::vec3& v) {
                return glm::vec3(std::fabs(v.x), std::fabs(v.y), std::fabs(v.z));
            }

            // Max component of a vec3 (absolute values).
            static float MaxComponent(const glm::vec3& v) {
                return std::max(std::max(std::fabs(v.x), std::fabs(v.y)), std::fabs(v.z));
            }

            // Returns true if the value is (almost) zero.
            static bool IsZero(float v, float eps = 1e-6f) {
                return std::fabs(v) < eps;
            }

            // Clamp value to [min, max].
            static float Clamp(float v, float min, float max) {
                return std::max(min, std::min(max, v));
            }
        };

        // Simple 3D ray structure.
        struct Ray {
            glm::vec3 origin;
            glm::vec3 direction; // expected normalized
        };

        // Simple 3D plane: normal + offset (signed distance from origin).
        struct Plane {
            glm::vec3 normal;
            float distance;
        };

    }
}
