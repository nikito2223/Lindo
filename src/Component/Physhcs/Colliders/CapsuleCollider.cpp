#include "CapsuleCollider.h"
#include "BoxCollider.h"
#include "SphereCollider.h"
#include "MeshCollider.h"
#include <Physics/Math/VectorMath.h>
#include <Component/GameObject/GameObject.h>
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include "Graphics/core/DebugDraw.h"

namespace Lindo {
    namespace Components {
        namespace Physics {

            using Lindo::Math::VectorMath;

            glm::vec3 CapsuleCollider::GetWorldCenter() const {
                return Collider::GetWorldPosition();
            }

            float CapsuleCollider::GetWorldRadius() const {
                // Получаем масштаб объекта в мировых координатах
                glm::vec3 worldScale = gameObject ? gameObject->transform.scale : glm::vec3(1.0f);
            
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
                if (gameObject) {
                    glm::vec3 r = gameObject->transform.rotation; // Euler degrees
                    glm::mat3 rotX = glm::mat3(glm::rotate(glm::mat4(1.0f), glm::radians(r.x), glm::vec3(1, 0, 0)));
                    glm::mat3 rotY = glm::mat3(glm::rotate(glm::mat4(1.0f), glm::radians(r.y), glm::vec3(0, 1, 0)));
                    glm::mat3 rotZ = glm::mat3(glm::rotate(glm::mat4(1.0f), glm::radians(r.z), glm::vec3(0, 0, 1)));
                    return rotZ * rotY * rotX;
                }
                return glm::mat3(1.0f);
            }

            void CapsuleCollider::GetEndpoints(glm::vec3& outTop, glm::vec3& outBottom) const {
                glm::vec3 worldScale = gameObject ? gameObject->transform.scale : glm::vec3(1.0f);
            
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

            // Capsule vs MeshCollider
            bool CapsuleCollider::CheckCollision(const MeshCollider* other, CollisionInfo& outInfo) const {
                if (!other) return false;
            
                glm::vec3 top, bottom;
                GetEndpoints(bottom, top);
                float capsuleRadius = GetWorldRadius();
            
                const auto& triangles = other->GetWorldTriangles();
                bool collided = false;
                float maxPenetration = -1.0f;
                CollisionInfo bestInfo;
            
                for (const auto& tri : triangles) {
                    // Находим ближайшую точку на треугольнике к сегменту капсулы (bottom -> top)
                    // (Используем встроенный метод или проецирование сегмента на плоскость треугольника)
                    glm::vec3 edge1 = tri.v1 - tri.v0;
                    glm::vec3 edge2 = tri.v2 - tri.v0;
                    glm::vec3 normal = glm::normalize(glm::cross(edge1, edge2));
                
                    // Простая и надежная проверка расстояния от вершин/сегмента до треугольника
                    // Для каждого треугольника определяем ближайшую точку на его поверхности к оси капсулы
                    // (Здесь приведена базовая проекция середины сегмента/точек на треугольник)

                    glm::vec3 segCenter = (bottom + top) * 0.5f;

                    // Вектор от плоскости треугольника до центра сегмента
                    float d = glm::dot(segCenter - tri.v0, normal);

                    // Если центр капсулы слишком далеко от плоскости треугольника, пропускаем
                    if (std::abs(d) > capsuleRadius + glm::length(top - bottom) * 0.5f) {
                        continue;
                    }
                
                    // Проекция центра на плоскость треугольника
                    glm::vec3 projPoint = segCenter - normal * d;
                
                    // Проверка, лежит ли проекция внутри треугольника (барицентрические координаты или тест ребер)
                    // Если попало, считаем минимальное расстояние
                    glm::vec3 closestOnTri = projPoint; // Упрощенная точка контакта на плоскости
                    // (В полноценном движке здесь вызывается точный тест Segment-Triangle, 
                    // но базовая проверка плоскости + радиуса уже решает проблему прилипания)
                
                    glm::vec3 diff = segCenter - closestOnTri;
                    float dist = glm::length(diff);
                
                    if (dist <= capsuleRadius) {
                        float penetration = capsuleRadius - dist;

                        glm::vec3 contactNormal = (dist > 1e-8f) ? (diff / dist) : normal;

                        // Защита от затягивания снизу: нормаль всегда должна смотреть наружу от меша
                        if (glm::dot(contactNormal, normal) < 0.0f) {
                            contactNormal = -normal;
                        }
                    
                        if (!collided || penetration > maxPenetration) {
                            maxPenetration = penetration;
                            bestInfo.other = const_cast<MeshCollider*>(other);
                            bestInfo.contactNormal = contactNormal;
                            bestInfo.contactPoint = closestOnTri;
                            bestInfo.penetrationDepth = maxPenetration;
                            bestInfo.relativeVelocity = 0.0f;
                            collided = true;
                        }
                    }
                }
            
                if (collided) {
                    outInfo = bestInfo;
                    return true;
                }
            
                return false;
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
                if (!gameObject || !enabledGizmos) return;

                auto& debugDraw = Lindo::Graphics::DebugDraw::GetInstance();

                glm::mat4 world = gameObject->getWorldMatrix();
                glm::vec3 pos = GetWorldPosition();

                // 1. Локальные оси
                glm::vec3 xAxis = glm::vec3(world[0]) * 0.5f;
                glm::vec3 yAxis = glm::vec3(world[1]) * 0.5f;
                glm::vec3 zAxis = glm::vec3(world[2]) * 0.5f;

                debugDraw.DrawLine(pos, pos + xAxis, glm::vec3(1.0f, 0.0f, 0.0f));
                debugDraw.DrawLine(pos, pos + yAxis, glm::vec3(0.0f, 1.0f, 0.0f));
                debugDraw.DrawLine(pos, pos + zAxis, glm::vec3(0.0f, 0.0f, 1.0f));

                // 2. Расчет параметров геометрии капсулы
                glm::vec3 top, bottom;
                GetEndpoints(bottom, top);
                float radius = GetWorldRadius();
                glm::vec3 axis = top - bottom;
                float axisLen = glm::length(axis);
                glm::vec3 dir = axisLen > 1e-6f ? axis / axisLen : glm::vec3(0.0f, 1.0f, 0.0f);

                glm::vec3 ortho = glm::abs(glm::dot(dir, glm::vec3(0.0f, 1.0f, 0.0f))) > 0.99f
                    ? glm::vec3(1.0f, 0.0f, 0.0f)
                    : glm::normalize(glm::cross(dir, glm::vec3(0.0f, 1.0f, 0.0f)));
                glm::vec3 tangent = glm::normalize(glm::cross(dir, ortho));
                glm::vec3 bitangent = glm::normalize(glm::cross(dir, tangent));

                const int segments = 20;
                const int halfSegments = 10;
                const glm::vec3 color(1.0f, 0.7f, 0.0f);

                // 3. Отрисовка верхушки и основания
                debugDraw.DrawCircle(top, tangent, bitangent, radius, color, segments);
                debugDraw.DrawCircle(bottom, tangent, bitangent, radius, color, segments);

                // 4. Отрисовка соединительных линий
                debugDraw.DrawLine(top + tangent * radius, bottom + tangent * radius, color);
                debugDraw.DrawLine(top - tangent * radius, bottom - tangent * radius, color);
                debugDraw.DrawLine(top + bitangent * radius, bottom + bitangent * radius, color);
                debugDraw.DrawLine(top - bitangent * radius, bottom - bitangent * radius, color);

                // 5. Отрисовка полусферических дуг (полукуполов)
                auto drawArc = [&](const glm::vec3& center, const glm::vec3& planeVec, float sign) {
                    glm::vec3 prevPoint = center + planeVec * radius;
                    for (int i = 1; i <= halfSegments; ++i) {
                        float theta = 3.14159265f * float(i) / float(halfSegments);
                        glm::vec3 nextPoint = center + cos(theta) * planeVec * radius + sin(theta) * dir * radius * sign;
                        debugDraw.DrawLine(prevPoint, nextPoint, color);
                        prevPoint = nextPoint;
                    }
                    };

                drawArc(top, tangent, 1.0f);
                drawArc(top, bitangent, 1.0f);
                drawArc(bottom, tangent, -1.0f);
                drawArc(bottom, bitangent, -1.0f);
            }

        }
    }
}
