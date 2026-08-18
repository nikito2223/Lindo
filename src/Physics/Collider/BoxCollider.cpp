#include "BoxCollider.h"
#include "SphereCollider.h"
#include "CapsuleCollider.h"
#include <Physics/Math/VectorMath.h>
#include <Component/GameObject/GameObject.h>
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <limits>

namespace Lindo {
    namespace Components {
        namespace Physics {

            glm::vec3 BoxCollider::GetWorldCenter() const {
                return Collider::GetWorldPosition(); // Использует offset из базового класса
            }

            glm::vec3 BoxCollider::GetWorldHalfExtents() const {
                glm::vec3 scale = GetWorldScale();
                return (size * scale) * 0.5f;
            }

            glm::mat3 BoxCollider::GetWorldRotationMatrix() const {
                if (gameObject) {
                    glm::vec3 r = gameObject->transform.rotation;
                    glm::mat3 rotX = glm::mat3(glm::rotate(glm::mat4(1.0f), glm::radians(r.x), glm::vec3(1, 0, 0)));
                    glm::mat3 rotY = glm::mat3(glm::rotate(glm::mat4(1.0f), glm::radians(r.y), glm::vec3(0, 1, 0)));
                    glm::mat3 rotZ = glm::mat3(glm::rotate(glm::mat4(1.0f), glm::radians(r.z), glm::vec3(0, 0, 1)));
                    return rotZ * rotY * rotX;
                }
                return glm::mat3(1.0f);
            }

            void BoxCollider::FitToAABB(const Lindo::Math::AABB& aabb) {
                // Строго устанавливаем размер и сдвиг по аналогии со SphereCollider
                size = aabb.max - aabb.min;
                offset = (aabb.max + aabb.min) * 0.5f;
            }

            Lindo::Math::AABB BoxCollider::GetAABB() const {
                glm::vec3 center = GetWorldCenter();
                glm::vec3 half = GetWorldHalfExtents();
                glm::mat3 rot = GetWorldRotationMatrix();

                // Проекция локальных OBB осей на мировые AABB оси
                glm::vec3 worldHalf(
                    std::abs(rot[0][0]) * half.x + std::abs(rot[1][0]) * half.y + std::abs(rot[2][0]) * half.z,
                    std::abs(rot[0][1]) * half.x + std::abs(rot[1][1]) * half.y + std::abs(rot[2][1]) * half.z,
                    std::abs(rot[0][2]) * half.x + std::abs(rot[1][2]) * half.y + std::abs(rot[2][2]) * half.z
                );

                return Lindo::Math::AABB(center - worldHalf, center + worldHalf);
            }

            bool BoxCollider::CheckCollision(Collider* other, CollisionInfo& outInfo) const {
                if (!other) return false;
                return other->CheckCollision(this, outInfo);
            }

            // Box vs Sphere
            bool BoxCollider::CheckCollision(const SphereCollider* other, CollisionInfo& outInfo) const {
                if (!other) return false;
                // Переадресуем в рабочий Sphere vs Box
                if (other->CheckCollision(this, outInfo)) {
                    // ИНВЕРСИЯ НОРМАЛИ: нормаль должна указывать от other к this
                    outInfo.contactNormal = -outInfo.contactNormal;
                    outInfo.other = const_cast<SphereCollider*>(other);
                    return true;
                }
                return false;
            }

            // Box vs Capsule
            bool BoxCollider::CheckCollision(const CapsuleCollider* other, CollisionInfo& outInfo) const {
                if (!other) return false;
                // Переадресуем в рабочий Capsule vs Box
                if (other->CheckCollision(this, outInfo)) {
                    // ИНВЕРСИЯ НОРМАЛИ: нормаль должна указывать от other к this
                    outInfo.contactNormal = -outInfo.contactNormal;
                    outInfo.other = const_cast<CapsuleCollider*>(other);
                    return true;
                }
                return false;
            }

            // Box vs Box (Исправленный алгоритм SAT в мировом пространстве)
            bool BoxCollider::CheckCollision(const BoxCollider* other, CollisionInfo& outInfo) const {
                if (!other) return false;

                glm::vec3 centerA = GetWorldCenter();
                glm::vec3 halfA = GetWorldHalfExtents();
                glm::mat3 rotA = GetWorldRotationMatrix();

                glm::vec3 centerB = other->GetWorldCenter();
                glm::vec3 halfB = other->GetWorldHalfExtents();
                glm::mat3 rotB = other->GetWorldRotationMatrix();

                glm::vec3 axes[15];
                int axisCount = 0;

                glm::vec3 aAxes[3] = { rotA[0], rotA[1], rotA[2] };
                for (int i = 0; i < 3; ++i) axes[axisCount++] = aAxes[i];

                glm::vec3 bAxes[3] = { rotB[0], rotB[1], rotB[2] };
                for (int i = 0; i < 3; ++i) axes[axisCount++] = bAxes[i];

                // Кросс-продукты осей для ребер
                for (int i = 0; i < 3; ++i) {
                    for (int j = 0; j < 3; ++j) {
                        glm::vec3 cross = glm::cross(aAxes[i], bAxes[j]);
                        if (glm::length(cross) > 1e-6f) {
                            axes[axisCount++] = glm::normalize(cross);
                        }
                    }
                }

                float minOverlap = std::numeric_limits<float>::max();
                glm::vec3 bestAxis(0.0f);

                for (int i = 0; i < axisCount; ++i) {
                    glm::vec3 axis = axes[i];
                    if (glm::length(axis) < 1e-6f) continue;
                    axis = glm::normalize(axis);

                    float projALen = std::fabs(glm::dot(axis, aAxes[0])) * halfA.x +
                        std::fabs(glm::dot(axis, aAxes[1])) * halfA.y +
                        std::fabs(glm::dot(axis, aAxes[2])) * halfA.z;

                    float projBLen = std::fabs(glm::dot(axis, bAxes[0])) * halfB.x +
                        std::fabs(glm::dot(axis, bAxes[1])) * halfB.y +
                        std::fabs(glm::dot(axis, bAxes[2])) * halfB.z;

                    float centerDist = std::fabs(glm::dot(axis, centerB - centerA));

                    if (centerDist > projALen + projBLen) {
                        return false; // Нашли разделяющую ось (нет столкновения)
                    }

                    float overlap = projALen + projBLen - centerDist;
                    if (overlap < minOverlap) {
                        minOverlap = overlap;
                        bestAxis = axis;
                    }
                }

                // Гарантируем, что нормаль направлена от Other (B) к This (A)
                if (glm::dot(bestAxis, centerA - centerB) < 0.0f) {
                    bestAxis = -bestAxis;
                }

                outInfo.other = const_cast<BoxCollider*>(other);
                outInfo.contactNormal = bestAxis;
                outInfo.penetrationDepth = minOverlap;
                outInfo.contactPoint = (centerA + centerB) * 0.5f;
                outInfo.relativeVelocity = 0.0f;
                return true;
            }

            void BoxCollider::OnDrawGizmos() {
                // Debug wireframe rendering hook
            }

        }
    }
}