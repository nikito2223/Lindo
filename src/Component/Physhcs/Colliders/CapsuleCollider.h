#pragma once

#include "Collider.h"
#include <Physics/Math/AABB.h>
#include <algorithm>

namespace Lindo {
    namespace Components {
        namespace Physics {

            class CapsuleCollider : public Collider {
            public:
                enum class Direction {
                    X = 0,
                    Y = 1,
                    Z = 2
                };

            private:
                float radius = 0.5f;
                float height = 2.0f; // ИСПРАВЛЕНО: 2.0f вместо 1.0f (при radius = 0.5f дает полноценную капсулу)
                Direction direction = Direction::Y;

            public:
                CapsuleCollider() = default;
                CapsuleCollider(float radius, float height, Direction dir = Direction::Y)
                    : radius(std::max(0.0001f, radius)),
                      height(std::max(0.0001f, height)),
                      direction(dir) {}

                CapsuleCollider(float radius, float height, int dir)
                    : radius(std::max(0.0001f, radius)),
                      height(std::max(0.0001f, height)),
                      direction(static_cast<Direction>(dir)) {}

                // ----- Configuration -----
                void SetRadius(float newRadius) { radius = std::max(0.0001f, newRadius); }
                float GetRadius() const { return radius; }

                void SetHeight(float newHeight) { height = std::max(0.0001f, newHeight); }
                float GetHeight() const { return height; }

                // Возвращает чистую длину центрального цилиндра (без учета полусфер)
                float GetCylinderHeight() const { return std::max(0.0f, height - (2.0f * radius)); }

                void SetDirection(Direction dir) { direction = dir; }
                void SetDirection(int dir) { direction = static_cast<Direction>(dir); }
                Direction GetDirection() const { return direction; }
                int GetDirectionInt() const { return static_cast<int>(direction); }

                // ----- Automatic Mesh / Bounds Fitting -----
                // Автоматически рассчитывает radius и height из AABB модели
                void FitToAABB(const Lindo::Math::AABB& aabb);

                // World-space radius (scaled by the gameObject's scale).
                float GetWorldRadius() const;

                // World-space rotation as a 3x3 matrix (from the gameObject).
                glm::mat3 GetWorldRotationMatrix() const;

                // World-space endpoints of the capsule's central segment.
                void GetEndpoints(glm::vec3& outTop, glm::vec3& outBottom) const;

                // World-space center of the capsule.
                glm::vec3 GetWorldCenter() const override;

                // World-space AABB.
                Lindo::Math::AABB GetAABB() const override;

                // ----- Collision queries -----
                bool CheckCollision(Collider* other, CollisionInfo& outInfo) const override;
                bool CheckCollision(const BoxCollider* other, CollisionInfo& outInfo) const override;
                bool CheckCollision(const SphereCollider* other, CollisionInfo& outInfo) const override;
                bool CheckCollision(const CapsuleCollider* other, CollisionInfo& outInfo) const override;
                bool CheckCollision(const MeshCollider* other, CollisionInfo& outInfo) const override; // <--- Добавляем

                // ----- Debug rendering -----
                void OnDrawGizmos() override;

            private:
                glm::vec3 GetClosestPointOnSegment(const glm::vec3& point) const;
                bool CapsuleCapsule(const CapsuleCollider* other, CollisionInfo& info) const;
            };

        }
    }
}