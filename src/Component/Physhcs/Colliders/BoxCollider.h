#pragma once

#include "Collider.h"
#include <Physics/Math/AABB.h>
#include <glm/glm.hpp>

namespace Lindo {
    namespace Components {
        namespace Physics {

            class BoxCollider : public Collider {
            private:
                glm::vec3 size = glm::vec3(1.0f);

            public:
                BoxCollider() = default;
                explicit BoxCollider(const glm::vec3& size) : size(glm::max(glm::vec3(0.0001f), size)) {}

                // ----- ��������� ������� -----
                void SetSize(const glm::vec3& newSize) { size = glm::max(glm::vec3(0.0001f), newSize); }
                glm::vec3 GetSize() const { return size; }

                // ----- �������������� �������� (Fix ������������ ����������) -----
                void FitToAABB(const Lindo::Math::AABB& aabb);

                // ----- ���������� ������� ��������� -----
                glm::vec3 GetWorldHalfExtents() const;
                glm::mat3 GetWorldRotationMatrix() const;
                glm::vec3 GetWorldCenter() const override;
                Lindo::Math::AABB GetAABB() const override;

                // ----- ������� ��������������� �������� -----
                bool CheckCollision(Collider* other, CollisionInfo& outInfo) const override;
                bool CheckCollision(const BoxCollider* other, CollisionInfo& outInfo) const override;
                bool CheckCollision(const SphereCollider* other, CollisionInfo& outInfo) const override;
                bool CheckCollision(const CapsuleCollider* other, CollisionInfo& outInfo) const override;
                bool CheckCollision(const MeshCollider* other, CollisionInfo& outInfo) const override;

                // ----- ��������� -----
                void OnDrawGizmos() override;
            };

        }
    }
}