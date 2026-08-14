#include "CapsuleCollider.h"
#include "BoxCollider.h"
#include "SphereCollider.h"
#include <Physics/Math/VectorMath.h>
#include <Component/GameObject/GameObject.h>
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>

namespace Lindo {
    namespace Components {
        namespace Physics {

            using Lindo::Math::VectorMath;

            glm::vec3 CapsuleCollider::GetWorldCenter() const {
                return Collider::GetWorldPosition();
            }

            float CapsuleCollider::GetWorldRadius() const {
                // Получаем масштаб объекта в мировых координатах
                glm::vec3 worldScale = owner ? owner->transform.scale : glm::vec3(1.0f);
            
                switch (direction) {
                case Direction::X:
                    // Для оси X радиус зависит только от Y и Z
                    return radius * std::max(std::abs(worldScale.y), std::abs(worldScale.z));
                case Direction::Z:
                    // Для оси Z радиус зависит только от X и Y
                    return radius * std::max(std::abs(worldScale.x), std::abs(worldScale.y));
                case Direction::Y:
                default:
                    // Для оси Y радиус зависит ТОЛЬКО от X и Z (высота Y больше не раздувает ширину!)
                    return radius * std::max(std::abs(worldScale.x), std::abs(worldScale.z));
                }
            }

            glm::mat3 CapsuleCollider::GetWorldRotationMatrix() const {
                if (owner) {
                    glm::vec3 r = owner->transform.rotation; // Euler degrees
                    glm::mat3 rotX = glm::mat3(glm::rotate(glm::mat4(1.0f), glm::radians(r.x), glm::vec3(1, 0, 0)));
                    glm::mat3 rotY = glm::mat3(glm::rotate(glm::mat4(1.0f), glm::radians(r.y), glm::vec3(0, 1, 0)));
                    glm::mat3 rotZ = glm::mat3(glm::rotate(glm::mat4(1.0f), glm::radians(r.z), glm::vec3(0, 0, 1)));
                    return rotZ * rotY * rotX;
                }
                return glm::mat3(1.0f);
            }

            void CapsuleCollider::GetEndpoints(glm::vec3& outTop, glm::vec3& outBottom) const {
                glm::vec3 worldScale = owner ? owner->transform.scale : glm::vec3(1.0f);
            
                // Берем масштаб именно вдоль центральной оси капсулы
                float axisScale = 1.0f;
                switch (direction) {
                case Direction::X: axisScale = std::abs(worldScale.x); break;
                case Direction::Y: axisScale = std::abs(worldScale.y); break;
                case Direction::Z: axisScale = std::abs(worldScale.z); break;
                }
            
                float worldHeight = height * axisScale;
                float worldRadius = GetWorldRadius();
            
                // Чистая длина внутреннего отрезка в мировых координатах
                float halfSegment = std::max(0.0f, (worldHeight * 0.5f) - worldRadius);
            
                glm::vec3 localOffset(0.0f);
                switch (direction) {
                case Direction::X: localOffset = glm::vec3(halfSegment, 0.0f, 0.0f); break;
                case Direction::Y: localOffset = glm::vec3(0.0f, halfSegment, 0.0f); break;
                case Direction::Z: localOffset = glm::vec3(0.0f, 0.0f, halfSegment); break;
                }
            
                glm::mat3 rot = GetWorldRotationMatrix();
                glm::vec3 center = GetWorldCenter();
            
                outTop = center + rot * localOffset;
                outBottom = center - rot * localOffset;
            }

            void CapsuleCollider::FitToAABB(const Lindo::Math::AABB& aabb) {
                glm::vec3 extents = aabb.max - aabb.min;
                        
                switch (direction) {
                case Direction::X:
                    radius = std::max(extents.y, extents.z) * 0.5f;
                    height = extents.x;
                    break;
                case Direction::Z:
                    radius = std::max(extents.x, extents.y) * 0.5f;
                    height = extents.z;
                    break;
                case Direction::Y:
                default:
                    radius = std::max(extents.x, extents.z) * 0.5f;
                    height = extents.y;
                    break;
                }
            
                // Защита: полная высота не может быть меньше диаметра (2 * R)
                radius = std::max(0.0001f, radius);
                height = std::max(height, radius * 2.0f);
            }

            Lindo::Math::AABB CapsuleCollider::GetAABB() const {
                glm::vec3 top, bottom;
                GetEndpoints(bottom, top);
                float r = GetWorldRadius();

                glm::vec3 min(
                    std::min(top.x, bottom.x) - r,
                    std::min(top.y, bottom.y) - r,
                    std::min(top.z, bottom.z) - r);
                glm::vec3 max(
                    std::max(top.x, bottom.x) + r,
                    std::max(top.y, bottom.y) + r,
                    std::max(top.z, bottom.z) + r);
                return Lindo::Math::AABB(min, max);
            }

            bool CapsuleCollider::CheckCollision(Collider* other, CollisionInfo& outInfo) const {
                if (!other) return false;
                return other->CheckCollision(this, outInfo);
            }

            // Capsule vs Box.
            bool CapsuleCollider::CheckCollision(const BoxCollider* other, CollisionInfo& outInfo) const {
                if (!other) return false;

                glm::vec3 boxCenter = other->GetWorldCenter();
                glm::vec3 half = other->GetWorldHalfExtents();
                glm::mat3 rot = other->GetWorldRotationMatrix();
                float capsuleRadius = GetWorldRadius();

                glm::vec3 top, bottom;
                GetEndpoints(bottom, top);

// Transform capsule segment into box local space.
                glm::vec3 localTop = glm::transpose(rot) * (top - boxCenter);
                glm::vec3 localBottom = glm::transpose(rot) * (bottom - boxCenter);

                // Closest point on the segment to the box, computed by first
                // clamping the segment end to the AABB, then projecting the
                // clamped point back onto the segment. This converges to the
                // true closest point for the capsule-vs-box test.
                glm::vec3 boxClosest = VectorMath::ClosestPointOnAABB(-half, half, localTop);
                float t;
                glm::vec3 segClosest = VectorMath::ClosestPointOnSegment(localBottom, localTop, boxClosest, t);

                glm::vec3 closestOnBox = VectorMath::ClosestPointOnAABB(-half, half, segClosest);
                glm::vec3 diff = segClosest - closestOnBox;
                float dist2 = glm::length(diff);

                if (dist2 > capsuleRadius) {
                    return false;
                }

                glm::vec3 normal = (dist2 > 1e-8f) ? (diff / dist2) : glm::vec3(0, 1, 0);
                normal = glm::normalize(rot * normal);

                outInfo.other = const_cast<BoxCollider*>(other);
                outInfo.contactNormal = normal;
                outInfo.contactPoint = boxCenter + rot * closestOnBox;
                outInfo.penetrationDepth = glm::max(0.0f, capsuleRadius - dist2);
                outInfo.relativeVelocity = 0.0f;
                return true;
            }

            // Capsule vs Sphere
            bool CapsuleCollider::CheckCollision(const SphereCollider* other, CollisionInfo& outInfo) const {
                if (!other) return false;
            
                glm::vec3 top, bottom;
                GetEndpoints(bottom, top);
                float capsuleRadius = GetWorldRadius();
                glm::vec3 sphereCenter = other->GetWorldCenter();
                float sphereRadius = other->GetWorldRadius();
            
                glm::vec3 closestOnSeg = VectorMath::ClosestPointOnSegment(bottom, top, sphereCenter);
                
                // ИСПРАВЛЕНО: вектор направлен от капсулы (препятствие) к сфере (игрок)
                glm::vec3 diff = sphereCenter - closestOnSeg;
                float dist = glm::length(diff);
                float combinedRadius = capsuleRadius + sphereRadius;
            
                if (dist > combinedRadius) {
                    return false;
                }
            
                glm::vec3 normal = (dist > 1e-8f) ? (diff / dist) : glm::vec3(0, 1, 0);
            
                outInfo.other = const_cast<SphereCollider*>(other);
                outInfo.contactNormal = normal;
                outInfo.contactPoint = closestOnSeg + normal * capsuleRadius;
                outInfo.penetrationDepth = glm::max(0.0f, combinedRadius - dist);
                outInfo.relativeVelocity = 0.0f;
                return true;
            }
            
            // Capsule vs Capsule
            bool CapsuleCollider::CapsuleCapsule(const CapsuleCollider* other, CollisionInfo& info) const {
                if (!other) return false;
            
                glm::vec3 topA, bottomA;
                GetEndpoints(bottomA, topA);
                glm::vec3 topB, bottomB;
                other->GetEndpoints(bottomB, topB);
            
                float radiusA = GetWorldRadius();
                float radiusB = other->GetWorldRadius();
                float combinedRadius = radiusA + radiusB;
            
                glm::vec3 closestA, closestB;
                VectorMath::ClosestPointsBetweenSegments(bottomA, topA, bottomB, topB, closestA, closestB);
            
                // ИСПРАВЛЕНО: вектор от капсулы A (препятствие) к капсуле B (игрок)
                glm::vec3 diff = closestB - closestA;
                float dist = glm::length(diff);
            
                if (dist > combinedRadius) {
                    return false;
                }
            
                glm::vec3 normal = (dist > 1e-8f) ? (diff / dist) : glm::vec3(0, 1, 0);
            
                info.other = const_cast<CapsuleCollider*>(other);
                info.contactNormal = normal;
                info.contactPoint = (closestA + closestB) * 0.5f;
                info.penetrationDepth = glm::max(0.0f, combinedRadius - dist);
                info.relativeVelocity = 0.0f;
                return true;
            }

            // Capsule vs Capsule.
            bool CapsuleCollider::CheckCollision(const CapsuleCollider* other, CollisionInfo& outInfo) const {
                return CapsuleCapsule(other, outInfo);
            }

            glm::vec3 CapsuleCollider::GetClosestPointOnSegment(const glm::vec3& point) const {
                glm::vec3 top, bottom;
                GetEndpoints(bottom, top);
                return VectorMath::ClosestPointOnSegment(bottom, top, point);
            }

            void CapsuleCollider::OnDrawGizmos() {
                // Debug wireframe rendering hook.
            }

        }
    }
}
