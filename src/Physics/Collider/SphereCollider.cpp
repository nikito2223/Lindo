#include "SphereCollider.h"
#include "BoxCollider.h"
#include "CapsuleCollider.h"
#include <Physics/Math/VectorMath.h>
#include <Component/GameObject/GameObject.h>
#include <algorithm>

namespace Lindo {
    namespace Components {
        namespace Physics {

            using Lindo::Math::VectorMath;

            glm::vec3 SphereCollider::GetWorldCenter() const {
                return Collider::GetWorldPosition();
            }

            float SphereCollider::GetWorldRadius() const {
                glm::vec3 scale = GetWorldScale();
                float maxScale = std::max(std::max(scale.x, scale.y), scale.z);
                return radius * std::max(maxScale, 0.0001f);
            }

            void Lindo::Components::Physics::SphereCollider::FitToAABB(const Lindo::Math::AABB& aabb) {
                glm::vec3 extents = aabb.max - aabb.min;

                // Чтобы сфера полностью охватывала объект, берем самую длинную сторону AABB и делим на 2
                this->radius = std::max({ extents.x, extents.y, extents.z }) * 0.5f;

                // Смещаем центр сферы в центр меша
                this->offset = (aabb.max + aabb.min) * 0.5f;
            }

            Lindo::Math::AABB SphereCollider::GetAABB() const {
                glm::vec3 center = GetWorldCenter();
                float r = GetWorldRadius();
                return Lindo::Math::AABB(center - glm::vec3(r), center + glm::vec3(r));
            }

            bool SphereCollider::CheckCollision(Collider* other, CollisionInfo& outInfo) const {
                if (!other) return false;
                return other->CheckCollision(this, outInfo);
            }

            // Sphere vs Box.
            bool SphereCollider::CheckCollision(const BoxCollider* other, CollisionInfo& outInfo) const {
                if (!other) return false;

                glm::vec3 boxCenter = other->GetWorldCenter();
                glm::vec3 half = other->GetWorldHalfExtents();
                glm::mat3 rot = other->GetWorldRotationMatrix();
                glm::vec3 sphereCenter = GetWorldCenter();
                float sphereRadius = GetWorldRadius();

                // Transform sphere center into box local space.
                glm::vec3 localCenter = glm::transpose(rot) * (sphereCenter - boxCenter);

                // Closest point on the box (local space) to the sphere center.
                glm::vec3 closest = VectorMath::ClosestPointOnAABB(-half, half, localCenter);
                glm::vec3 diff = localCenter - closest;
                float distSq = glm::dot(diff, diff);

                if (distSq > sphereRadius * sphereRadius) {
                    return false; // No collision
                }

                glm::vec3 contactLocal;
                float penetration;
                if (distSq > 1e-8f) {
                    contactLocal = closest;
                    float dist = std::sqrt(distSq);
                    penetration = sphereRadius - dist;
                } else {
                    // Sphere center inside the box: use the face of minimum
                    // penetration to push the sphere out.
                    contactLocal = localCenter;
                    glm::vec3 distances(
                        half.x - std::fabs(localCenter.x),
                        half.y - std::fabs(localCenter.y),
                        half.z - std::fabs(localCenter.z));
                    if (distances.x < distances.y && distances.x < distances.z) {
                        contactLocal.x = (localCenter.x < 0) ? -half.x : half.x;
                    } else if (distances.y < distances.z) {
                        contactLocal.y = (localCenter.y < 0) ? -half.y : half.y;
                    } else {
                        contactLocal.z = (localCenter.z < 0) ? -half.z : half.z;
                    }
                    penetration = sphereRadius + glm::length(localCenter - contactLocal);
                }

                outInfo.other = const_cast<BoxCollider*>(other);
                outInfo.contactNormal = glm::normalize(localCenter - contactLocal);
                if (glm::length(outInfo.contactNormal) < 1e-6f) {
                    outInfo.contactNormal = glm::vec3(0, 1, 0);
                }
                outInfo.contactNormal = glm::normalize(rot * outInfo.contactNormal);
                outInfo.contactPoint = boxCenter + rot * contactLocal;
                outInfo.penetrationDepth = glm::max(0.0f, penetration);
                outInfo.relativeVelocity = 0.0f;
                return true;
            }

            // Sphere vs Capsule
            bool SphereCollider::CheckCollision(const CapsuleCollider* other, CollisionInfo& outInfo) const {
                if (!other) return false;
            
                glm::vec3 top, bottom;
                other->GetEndpoints(bottom, top);
                float capsuleRadius = other->GetWorldRadius();
                glm::vec3 sphereCenter = GetWorldCenter();
                float sphereRadius = GetWorldRadius();
            
                glm::vec3 closestOnSeg = VectorMath::ClosestPointOnSegment(bottom, top, sphereCenter);

                // ИСПРАВЛЕНО: вектор направлен от сферы (препятствия) к капсуле (игроку)
                glm::vec3 diff = closestOnSeg - sphereCenter; 
                float dist = glm::length(diff);
                float combinedRadius = sphereRadius + capsuleRadius;
            
                if (dist > combinedRadius) {
                    return false;
                }
            
                glm::vec3 normal = (dist > 1e-8f) ? (diff / dist) : glm::vec3(0, 1, 0);
            
                outInfo.other = const_cast<CapsuleCollider*>(other);
                outInfo.contactNormal = normal;
                outInfo.contactPoint = sphereCenter + normal * sphereRadius;
                outInfo.penetrationDepth = glm::max(0.0f, combinedRadius - dist);
                outInfo.relativeVelocity = 0.0f;
                return true;
            }

            // Sphere vs Sphere
            bool SphereCollider::CheckCollision(const SphereCollider* other, CollisionInfo& outInfo) const {
                if (!other) return false;
            
                glm::vec3 centerA = GetWorldCenter();
                glm::vec3 centerB = other->GetWorldCenter();
                float radiusA = GetWorldRadius();
                float radiusB = other->GetWorldRadius();
            
                // ИСПРАВЛЕНО: вектор направлен от Sphere A (препятствие) к Sphere B (игрок)
                glm::vec3 delta = centerB - centerA;
                float distSq = glm::dot(delta, delta);
                float radiusSum = radiusA + radiusB;
            
                if (distSq > radiusSum * radiusSum) {
                    return false;
                }
            
                float dist = std::sqrt(distSq);
                glm::vec3 normal = (dist > 1e-8f) ? (delta / dist) : glm::vec3(0, 1, 0);
            
                outInfo.other = const_cast<SphereCollider*>(other);
                outInfo.contactNormal = normal;
                outInfo.contactPoint = centerA + normal * radiusA;
                outInfo.penetrationDepth = glm::max(0.0f, radiusSum - dist);
                outInfo.relativeVelocity = 0.0f;
                return true;
            }

            glm::vec3 SphereCollider::ClosestPointOnBox(const BoxCollider* box, const glm::vec3& sphereCenter) const {
                glm::vec3 boxCenter = box->GetWorldCenter();
                glm::vec3 half = box->GetWorldHalfExtents();
                glm::mat3 rot = box->GetWorldRotationMatrix();
                glm::vec3 localCenter = glm::transpose(rot) * (sphereCenter - boxCenter);
                glm::vec3 localClosest = VectorMath::ClosestPointOnAABB(-half, half, localCenter);
                return boxCenter + rot * localClosest;
            }

            void SphereCollider::OnDrawGizmos() {
                // Debug wireframe rendering hook.
            }

        }
    }
}
