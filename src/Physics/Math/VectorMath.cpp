#include "VectorMath.h"

namespace Lindo {
    namespace Math {

        glm::vec3 VectorMath::ClosestPointOnSegment(
            const glm::vec3& a, const glm::vec3& b, const glm::vec3& p, float& outT) {
            glm::vec3 ab = b - a;
            float abLenSq = glm::dot(ab, ab);
            if (abLenSq < 1e-8f) {
                outT = 0.0f;
                return a;
            }
            float t = glm::dot(p - a, ab) / abLenSq;
            outT = Clamp(t, 0.0f, 1.0f);
            return a + ab * outT;
        }

        glm::vec3 VectorMath::ClosestPointOnSegment(const glm::vec3& a, const glm::vec3& b, const glm::vec3& p) {
            float t;
            return ClosestPointOnSegment(a, b, p, t);
        }

        float VectorMath::SquaredDistancePointSegment(
            const glm::vec3& a, const glm::vec3& b, const glm::vec3& p) {
            glm::vec3 closest = ClosestPointOnSegment(a, b, p);
            glm::vec3 diff = p - closest;
            return glm::dot(diff, diff);
        }

        float VectorMath::DistancePointSegment(
            const glm::vec3& a, const glm::vec3& b, const glm::vec3& p) {
            return std::sqrt(SquaredDistancePointSegment(a, b, p));
        }

        void VectorMath::ClosestPointsBetweenSegments(
            const glm::vec3& p1, const glm::vec3& p2,
            const glm::vec3& q1, const glm::vec3& q2,
            glm::vec3& outP, glm::vec3& outQ) {
            glm::vec3 d1 = p2 - p1;
            glm::vec3 d2 = q2 - q1;
            glm::vec3 r = p1 - q1;
            float a = glm::dot(d1, d1);
            float e = glm::dot(d2, d2);
            float f = glm::dot(d2, r);

            float s, t;
            const float eps = 1e-6f;

            if (a <= eps && e <= eps) {
                outP = p1;
                outQ = q1;
                return;
            }
            if (a <= eps) {
                s = 0.0f;
                t = Clamp(f / e, 0.0f, 1.0f);
            } else {
                float c = glm::dot(d1, r);
                if (e <= eps) {
                    t = 0.0f;
                    s = Clamp(-c / a, 0.0f, 1.0f);
                } else {
                    float b = glm::dot(d1, d2);
                    float denom = a * e - b * b;
                    if (denom > eps) {
                        s = Clamp((b * f - c * e) / denom, 0.0f, 1.0f);
                    } else {
                        s = 0.0f;
                    }
                    t = (b * s + f) / e;
                    if (t < 0.0f) {
                        t = 0.0f;
                        s = Clamp(-c / a, 0.0f, 1.0f);
                    } else if (t > 1.0f) {
                        t = 1.0f;
                        s = Clamp((b - c) / a, 0.0f, 1.0f);
                    }
                }
            }

            outP = p1 + d1 * s;
            outQ = q1 + d2 * t;
        }

        float VectorMath::SquaredDistanceSegments(
            const glm::vec3& p1, const glm::vec3& p2,
            const glm::vec3& q1, const glm::vec3& q2) {
            glm::vec3 p, q;
            ClosestPointsBetweenSegments(p1, p2, q1, q2, p, q);
            glm::vec3 diff = p - q;
            return glm::dot(diff, diff);
        }

        glm::vec3 VectorMath::ClosestPointOnAABB(
            const glm::vec3& min, const glm::vec3& max, const glm::vec3& p) {
            return glm::vec3(
                Clamp(p.x, min.x, max.x),
                Clamp(p.y, min.y, max.y),
                Clamp(p.z, min.z, max.z));
        }

    }
}
