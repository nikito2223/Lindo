#include "BoxCollider.h"
#include "SphereCollider.h"
#include "CapsuleCollider.h"
#include <Physics/Math/VectorMath.h>
#include <Component/GameObject/GameObject.h>
#include <algorithm>

namespace Lindo {
    namespace Components {
        namespace Physics {

            using Lindo::Math::VectorMath;

            glm::mat3 BoxCollider::GetWorldRotationMatrix() const {
                if (owner) {
                    glm::mat4 model = owner->getWorldMatrix();
                    // Extract linear (rotation+scale) part, then normalize columns
                    // to get pure rotation.
                    glm::mat3 rot(
                        glm::normalize(glm::vec3(model[0])),
                        glm::normalize(glm::vec3(model[1])),
                        glm::normalize(glm::vec3(model[2])));
                    return rot;
                }
                return glm::mat3(1.0f);
            }

            glm::vec3 BoxCollider::GetWorldHalfExtents() const {
                glm::vec3 scale = GetWorldScale();
                return size * scale * 0.5f;
            }

            glm::vec3 BoxCollider::TransformPoint(const glm::vec3& localPoint) const {
                glm::vec3 center = GetWorldCenter();
                glm::mat3 rot = GetWorldRotationMatrix();
                glm::vec3 worldScale = GetWorldScale();
                return center + rot * (localPoint * worldScale);
            }

            void BoxCollider::GetAABB(glm::vec3& outMin, glm::vec3& outMax) const {
                glm::vec3 half = GetWorldHalfExtents();
                glm::vec3 center = GetWorldCenter();
                glm::mat3 rot = GetWorldRotationMatrix();

                // Compute the AABB of the rotated box in world space.
                glm::vec3 extents(
                    std::fabs(rot[0][0]) * half.x + std::fabs(rot[1][0]) * half.y + std::fabs(rot[2][0]) * half.z,
                    std::fabs(rot[0][1]) * half.x + std::fabs(rot[1][1]) * half.y + std::fabs(rot[2][1]) * half.z,
                    std::fabs(rot[0][2]) * half.x + std::fabs(rot[1][2]) * half.y + std::fabs(rot[2][2]) * half.z);

                outMin = center - extents;
                outMax = center + extents;
            }

            Lindo::Math::AABB BoxCollider::GetAABB() const {
                glm::vec3 min, max;
                GetAABB(min, max);
                return Lindo::Math::AABB(min, max);
            }

            std::vector<glm::vec3> BoxCollider::GetWorldVertices() const {
                glm::vec3 half = GetWorldHalfExtents();
                glm::mat3 rot = GetWorldRotationMatrix();
                glm::vec3 center = GetWorldCenter();

                std::vector<glm::vec3> corners(8);
                for (int i = 0; i < 8; ++i) {
                    glm::vec3 local(
                        (i & 1) ? half.x : -half.x,
                        (i & 2) ? half.y : -half.y,
                        (i & 4) ? half.z : -half.z);
                    corners[i] = center + rot * local;
                }
                return corners;
            }

            bool BoxCollider::CheckCollision(Collider* other, CollisionInfo& outInfo) const {
                if (!other) return false;
                return other->CheckCollision(this, outInfo);
            }

            // Box vs Box using the Separating Axis Theorem (SAT).
            bool BoxCollider::CheckCollision(const BoxCollider* other, CollisionInfo& outInfo) const {
                if (!other) return false;

                glm::vec3 centerA = GetWorldCenter();
                glm::vec3 halfA = GetWorldHalfExtents();
                glm::mat3 rotA = GetWorldRotationMatrix();

                glm::vec3 centerB = other->GetWorldCenter();
                glm::vec3 halfB = other->GetWorldHalfExtents();
                glm::mat3 rotB = other->GetWorldRotationMatrix();

                // Compute the rotation matrix that expresses B in A's frame.
                glm::mat3 R = glm::transpose(rotA) * rotB;
                glm::vec3 translation = centerB - centerA;
                glm::vec3 t = glm::transpose(rotA) * translation;

                // Candidate separating axes (15 total):
                // 3 from A's axes, 3 from B's axes, 9 cross products.
                glm::vec3 axes[15];
                int axisCount = 0;

                // A's local axes in A's frame are just unit axes.
                glm::vec3 aAxes[3] = { glm::vec3(1,0,0), glm::vec3(0,1,0), glm::vec3(0,0,1) };
                for (int i = 0; i < 3; ++i) axes[axisCount++] = aAxes[i];

                // B's local axes expressed in A's frame.
                for (int i = 0; i < 3; ++i) {
                    axes[axisCount++] = glm::vec3(R[0][i], R[1][i], R[2][i]);
                }

                // Cross products of every A axis with every B axis.
                for (int i = 0; i < 3; ++i) {
                    for (int j = 0; j < 3; ++j) {
                        glm::vec3 cross = glm::cross(aAxes[i],
                            glm::vec3(R[0][j], R[1][j], R[2][j]));
                        if (glm::length(cross) > 1e-6f) {
                            axes[axisCount++] = cross;
                        }
                    }
                }

float minOverlap = std::numeric_limits<float>::max();
                glm::vec3 bestAxis(0.0f);

                for (int i = 0; i < axisCount; ++i) {
                    glm::vec3 axis = axes[i];
                    if (glm::length(axis) < 1e-6f) continue;
                    axis = glm::normalize(axis);

                    // Project each box onto the axis using its oriented extents.
                    float projALen = std::fabs(glm::dot(axis, glm::vec3(rotA[0]))) * halfA.x +
                                     std::fabs(glm::dot(axis, glm::vec3(rotA[1]))) * halfA.y +
                                     std::fabs(glm::dot(axis, glm::vec3(rotA[2]))) * halfA.z;
                    float projBLen = std::fabs(glm::dot(axis, glm::vec3(rotB[0]))) * halfB.x +
                                     std::fabs(glm::dot(axis, glm::vec3(rotB[1]))) * halfB.y +
                                     std::fabs(glm::dot(axis, glm::vec3(rotB[2]))) * halfB.z;

                    // Distance between centers projected onto the axis.
                    float centerDist = std::fabs(glm::dot(axis, centerB - centerA));

                    if (centerDist > projALen + projBLen) {
                        // Separating axis found -> no collision.
                        return false;
                    }

                    float overlap = projALen + projBLen - centerDist;
                    if (overlap < minOverlap) {
                        minOverlap = overlap;
                        bestAxis = (glm::dot(axis, centerB - centerA) > 0.0f) ? axis : -axis;
                    }
                }

                // No separating axis found -> collision.
                outInfo.other = const_cast<BoxCollider*>(other);
                outInfo.contactNormal = bestAxis;
                outInfo.penetrationDepth = minOverlap;
                outInfo.contactPoint = (centerA + centerB) * 0.5f;
                outInfo.relativeVelocity = 0.0f;
                return true;
            }

            // Box vs Sphere.
            bool BoxCollider::CheckCollision(const SphereCollider* other, CollisionInfo& outInfo) const {
                if (!other) return false;

                glm::vec3 boxCenter = GetWorldCenter();
                glm::vec3 half = GetWorldHalfExtents();
                glm::mat3 rot = GetWorldRotationMatrix();
glm::vec3 sphereCenter = other->GetWorldCenter();
                float sphereRadius = other->GetWorldRadius();

                // Transform sphere center into box local space.
                glm::vec3 localCenter = glm::transpose(rot) * (sphereCenter - boxCenter);

                // Closest point on the box (in local space) to the sphere center.
                glm::vec3 closest = VectorMath::ClosestPointOnAABB(-half, half, localCenter);
                glm::vec3 diff = localCenter - closest;
                float distSq = glm::dot(diff, diff);

                if (distSq > sphereRadius * sphereRadius) {
                    return false; // No collision
                }

                // Collision detected. Compute normal and penetration.
                glm::vec3 contactLocal;
                float penetration;
                if (distSq > 1e-8f) {
                    contactLocal = closest;
                    float dist = std::sqrt(distSq);
                    penetration = sphereRadius - dist;
                } else {
                    // Sphere center inside box: push out along the axis of
                    // minimum penetration.
                    glm::vec3 penetrationVec(-half.x - localCenter.x, -half.y - localCenter.y, -half.z - localCenter.z);
                    penetrationVec.x = std::max(penetrationVec.x, half.x - localCenter.x);
                    penetrationVec.y = std::max(penetrationVec.y, half.y - localCenter.y);
                    penetrationVec.z = std::max(penetrationVec.z, half.z - localCenter.z);
                    // Find the axis with the smallest penetration magnitude.
                    if (std::fabs(penetrationVec.x) <= std::fabs(penetrationVec.y) &&
                        std::fabs(penetrationVec.x) <= std::fabs(penetrationVec.z)) {
                        contactLocal = glm::vec3((localCenter.x < 0) ? -half.x : half.x, localCenter.y, localCenter.z);
                    } else if (std::fabs(penetrationVec.y) <= std::fabs(penetrationVec.z)) {
                        contactLocal = glm::vec3(localCenter.x, (localCenter.y < 0) ? -half.y : half.y, localCenter.z);
                    } else {
                        contactLocal = glm::vec3(localCenter.x, localCenter.y, (localCenter.z < 0) ? -half.z : half.z);
                    }
                    glm::vec3 innerToCenter = localCenter - contactLocal;
                    penetration = sphereRadius + glm::length(innerToCenter);
                }

                outInfo.other = const_cast<SphereCollider*>(other);
                outInfo.contactNormal = glm::normalize(rot * (localCenter - contactLocal));
                if (glm::length(outInfo.contactNormal) < 1e-6f) {
                    outInfo.contactNormal = glm::vec3(0, 1, 0);
                }
                outInfo.contactPoint = boxCenter + rot * contactLocal;
                outInfo.penetrationDepth = glm::max(0.0f, penetration);
                outInfo.relativeVelocity = 0.0f;
                return true;
            }

// Box vs Capsule.
            // Works by finding the closest point on the capsule's medial
            // segment to the (oriented) box, then testing the distance against
            // the capsule radius. This is performed in the box's local space so
            // that the box becomes an axis-aligned box.
            bool BoxCollider::CheckCollision(const CapsuleCollider* other, CollisionInfo& outInfo) const {
                if (!other) return false;

                glm::vec3 boxCenter = GetWorldCenter();
                glm::vec3 half = GetWorldHalfExtents();
                glm::mat3 rot = GetWorldRotationMatrix();

                glm::vec3 top, bottom;
                other->GetEndpoints(bottom, top);
                float capsuleRadius = other->GetWorldRadius();

                // Transform the capsule segment into box local space so the box
                // is axis-aligned before querying.
                glm::vec3 localTop = glm::transpose(rot) * (top - boxCenter);
                glm::vec3 localBottom = glm::transpose(rot) * (bottom - boxCenter);

                // A capsule is a Minkowski sum of its segment and a sphere of
                // radius `capsuleRadius`. The capsule intersects the box iff the
                // distance from the segment to the box is <= capsuleRadius. We
                // compute the closest point on the segment to the box iteratively
                // (the box is treated as a sphere of growing radius centered on
                // the closest point of the box to the segment).
                glm::vec3 closestOnBox = VectorMath::ClosestPointOnAABB(-half, half, localBottom);
                float t;
                glm::vec3 closestOnSeg = VectorMath::ClosestPointOnSegment(localBottom, localTop, closestOnBox, t);

                // Distance from the closest segment point to the box.
                glm::vec3 boxClosest = VectorMath::ClosestPointOnAABB(-half, half, closestOnSeg);
                glm::vec3 diff = closestOnSeg - boxClosest;
                float dist = glm::length(diff);

                if (dist > capsuleRadius) {
                    return false;
                }

                // Build the contact normal from the box surface toward the
                // capsule segment (in box local space), then rotate to world.
                glm::vec3 localNormal = (dist > 1e-8f) ? (diff / dist)
                                                       : glm::vec3(0.0f, 1.0f, 0.0f);
                glm::vec3 worldNormal = glm::normalize(rot * localNormal);
                if (glm::length(worldNormal) < 1e-6f) {
                    worldNormal = glm::vec3(0, 1, 0);
                }

                outInfo.other = const_cast<CapsuleCollider*>(other);
                outInfo.contactNormal = worldNormal;
                outInfo.contactPoint = boxCenter + rot * boxClosest;
                outInfo.penetrationDepth = glm::max(0.0f, capsuleRadius - dist);
                outInfo.relativeVelocity = 0.0f;
                return true;
            }

            void BoxCollider::OnDrawGizmos() {
                // Debug wireframe rendering hook (no-op in production, but
                // can be wired to a debug draw system).
            }

        }
    }
}
