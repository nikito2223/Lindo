#pragma once

#include <glm/glm.hpp>
#include <vector>
#include <functional>
#include <algorithm>
#include <Component/Component.h>
#include <Component/GameObject/GameObject.h>
#include <Physics/Math/AABB.h>

namespace Lindo {
    namespace Components {
        namespace Physics {

            // Forward declarations of all collider types.
            class Collider;
            class BoxCollider;
            class SphereCollider;
            class CapsuleCollider;
            class MeshCollider;

            // Physics material properties that can be configured per-collider.
            // These feed into the collision response (impulse resolution).
            struct PhysicsMaterial {
                float density = 1.0f;      // mass scaling factor
                float friction = 0.5f;     // surface friction [0, 1]
                float restitution = 0.2f;  // bounciness [0, 1]

                PhysicsMaterial() = default;
                PhysicsMaterial(float friction, float restitution)
                    : friction(friction), restitution(restitution) {}
            };

            // Result of a narrow-phase collision query.
            struct CollisionInfo {
                Lindo::Components::Physics::Collider* other = nullptr;
                glm::vec3 contactPoint{0.0f};
                glm::vec3 contactNormal{0.0f}; // points from `other` toward `this`
                float penetrationDepth = 0.0f;
                float relativeVelocity = 0.0f;
            };

            enum class CollisionEvent {
                Enter,
                Stay,
                Exit
            };

            // Base class for all collider components.
            // Derives from World::Component so it can be attached via
            // GameObject::addComponent<T>().
            class Collider : public World::Component {
            protected:
                // Behavior flags.
                bool isTrigger = false;
                bool isEnabled = true;

                // Local-space offset of the collider relative to its gameObject.
                glm::vec3 offset = glm::vec3(0.0f);

                // Local-space scale override (defaults to gameObject transform scale).
                glm::vec3 colliderScale = glm::vec3(1.0f);
                bool usegameObjectScale = true;

                // Physics material properties.
                PhysicsMaterial material;

                // Current collisions / triggers for event tracking.
                std::vector<Collider*> currentCollisions;
                std::vector<Collider*> currentTriggers;

                // Collision callbacks.
                std::function<void(const CollisionInfo&)> onCollisionEnterCallback;
                std::function<void(const CollisionInfo&)> onCollisionStayCallback;
                std::function<void(const CollisionInfo&)> onCollisionExitCallback;

                // Trigger callbacks.
                std::function<void(Collider*)> onTriggerEnterCallback;
                std::function<void(Collider*)> onTriggerStayCallback;
                std::function<void(Collider*)> onTriggerExitCallback;

            public:
                Collider() = default;
                virtual ~Collider();

                void OnStart() override;
                void OnDestroy() override;

                // ----- Behavior configuration -----
                void SetTrigger(bool trigger) { isTrigger = trigger; }
                bool IsTrigger() const { return isTrigger; }

                void SetEnabled(bool enabled) { isEnabled = enabled; }
                bool IsEnabled() const { return isEnabled; }

                // ----- Local offset -----
                void SetOffset(const glm::vec3& newOffset) { offset = newOffset; }
                glm::vec3 GetOffset() const { return offset; }

                // ----- Local scale -----
                void SetColliderScale(const glm::vec3& scale) { colliderScale = scale; usegameObjectScale = false; }
                glm::vec3 GetColliderScale() const;
                void SetUsegameObjectScale(bool use) { usegameObjectScale = use; }
                bool GetUsegameObjectScale() const { return usegameObjectScale; }

                // ----- Physics material -----
                void SetFriction(float friction) { material.friction = glm::clamp(friction, 0.0f, 1.0f); }
                float GetFriction() const { return material.friction; }
                void SetRestitution(float restitution) { material.restitution = glm::clamp(restitution, 0.0f, 1.0f); }
                float GetRestitution() const { return material.restitution; }
                void SetDensity(float density) { material.density = std::max(0.0f, density); }
                float GetDensity() const { return material.density; }
                const PhysicsMaterial& GetMaterial() const { return material; }
                void SetMaterial(const PhysicsMaterial& mat) { material = mat; }

                // ----- World-space state -----
                // World-space center of the collider (gameObject position + offset).
                virtual glm::vec3 GetWorldCenter() const;
                // World-space scale (gameObject scale * collider scale when configured).
                virtual glm::vec3 GetWorldScale() const;
                // World-space position (gameObject world position + offset).
                virtual glm::vec3 GetWorldPosition() const;
                // Computed world-space AABB for broad-phase testing.
                virtual Lindo::Math::AABB GetAABB() const = 0;

                // ----- Narrow-phase collision queries -----
                virtual bool CheckCollision(Collider* other, CollisionInfo& outInfo) const = 0;
                virtual bool CheckCollision(const BoxCollider* other, CollisionInfo& outInfo) const = 0;
                virtual bool CheckCollision(const SphereCollider* other, CollisionInfo& outInfo) const = 0;
                virtual bool CheckCollision(const CapsuleCollider* other, CollisionInfo& outInfo) const = 0;
                virtual bool CheckCollision(const MeshCollider* other, CollisionInfo& outInfo) const = 0; // <--- Новыи метод

                // ----- Event dispatch -----
                void OnCollisionEnter(Collider* other, const CollisionInfo& info);
                void OnCollisionStay(Collider* other, const CollisionInfo& info);
                void OnCollisionExit(Collider* other);
                void OnTriggerEnter(Collider* other);
                void OnTriggerStay(Collider* other);
                void OnTriggerExit(Collider* other);

                // ----- Callback registration -----
                void SetCollisionCallback(CollisionEvent event, std::function<void(const CollisionInfo&)> callback);
                void SetTriggerCallback(CollisionEvent event, std::function<void(Collider*)> callback);

                // ----- Current collision lists -----
                const std::vector<Collider*>& GetCurrentCollisions() const { return currentCollisions; }
                const std::vector<Collider*>& GetCurrentTriggers() const { return currentTriggers; }

                // ----- Debug rendering -----
                virtual void OnDrawGizmos() override = 0;

            protected:
                // Internal helpers for collision-list management.
                void AddCollision(Collider* other);
                void RemoveCollision(Collider* other);
                void AddTrigger(Collider* other);
                void RemoveTrigger(Collider* other);
            };

        }
    }
}
