#pragma once

#include <glm/glm.hpp>
#include <vector>
#include <memory>
#include <Physics/Math/AABB.h>
#include <Physics/Math/VectorMath.h>

namespace Lindo {
    namespace Components {
        namespace Physics {
            class Collider;
            class RigidBody;
            class GravityField;

            // A frame of contact data produced by the narrow phase.
            struct Contact {
                Collider* colliderA;
                Collider* colliderB;
                glm::vec3 point;
                glm::vec3 normal;   // points from A toward B
                float penetration;
                float friction;
                float restitution;
            };

            // Central physics simulation orchestrator.
            //
            // Responsibilities:
            //   - Register / unregister colliders and rigid bodies.
            //   - Broad-phase overlap culling via world AABBs.
            //   - Narrow-phase collision detection (delegates to colliders).
            //   - Collision response: impulse-based resolution with friction
            //     and restitution, plus trigger event dispatch.
            //   - Gravity application via configurable gravity fields.
            //
            // This is a singleton-style manager that the application drives
            // once per frame (see Step()).
            class PhysicsSystem {
            public:
                static PhysicsSystem& GetInstance();

                PhysicsSystem() = default;
                ~PhysicsSystem() = default;

                // Non-copyable.
                PhysicsSystem(const PhysicsSystem&) = delete;
                PhysicsSystem& operator=(const PhysicsSystem&) = delete;

                // ----- World management -----
                void RegisterCollider(Collider* collider);
                void UnregisterCollider(Collider* collider);
                void RegisterRigidBody(RigidBody* body);
                void UnregisterRigidBody(RigidBody* body);
                void RegisterGravityField(GravityField* field);
                void UnregisterGravityField(GravityField* field);

                // ----- Configuration -----
                void SetGlobalGravity(const glm::vec3& gravity);
                glm::vec3 GetGlobalGravity() const;
                void SetIterations(int iterations);
                int GetIterations() const;
                void SetVelocityIterations(int iterations);
                void SetPositionIterations(int iterations);
                void SetSolver(bool enable) { useSolver = enable; }
                void SetMaxPenetration(float maxPen) { maxPenetration = maxPen; }

                // ----- Per-frame stepping -----
                // Advances the entire simulation by deltaTime seconds, applying
                // gravity fields, integrating rigid bodies, detecting collisions,
                // and resolving them with an impulse solver.
                void Step();

                // ----- Spatial queries -----
                bool Raycast(const glm::vec3& origin, const glm::vec3& direction,
                    float maxDistance, Collider*& outHit, glm::vec3& outPoint,
                    glm::vec3& outNormal, float& outDistance) const;
                bool OverlapPoint(const glm::vec3& point, Collider*& outHit) const;
                std::vector<Collider*> OverlapSphere(const glm::vec3& center, float radius) const;
                std::vector<Collider*> OverlapBox(const glm::vec3& center, const glm::vec3& halfExtents) const;

                // ----- Accessors -----
                const std::vector<Collider*>& GetColliders() const { return colliders; }
                const std::vector<RigidBody*>& GetRigidBodies() const { return rigidBodies; }
                void Clear();

            private:
                struct BroadPair {
                    Collider* a;
                    Collider* b;
                };

                // Broad phase: returns candidate pairs whose AABBs overlap.
                std::vector<BroadPair> BroadPhase() const;

                // Narrow phase: computes contact data for a candidate pair.
                bool NarrowPhase(Collider* a, Collider* b, Contact& outContact) const;

// Resolution: applies impulses to resolve overlapping bodies.
                void ResolveContacts(std::vector<Contact>& contacts);

                // Compute the effective gravity at a body's position.
                glm::vec3 ComputeGravityAt(const glm::vec3& worldPos) const;

                // Compute combined friction / restitution for a contact.
                void ComputeContactProperties(const Contact& contact,
                    float& outFriction, float& outRestitution) const;

                // Event dispatch (enter/stay/exit) for a pair.
                void UpdateCollisionEvents(Collider* a, Collider* b, const Contact& contact);

                // Registered entities.
                std::vector<Collider*> colliders;
                std::vector<RigidBody*> rigidBodies;
                std::vector<GravityField*> gravityFields;

                // Simulation parameters.
                glm::vec3 globalGravity{ 0.0f, -9.81f, 0.0f };
                int solverIterations = 8;
                bool useSolver = true;
                float maxPenetration = 0.05f;
                float slop = 0.005f;      // allowed penetration before resolution
                float bias = 0.2f;        // positional correction bias
                float stepAccumulator = 0.0f;

                void StepFixed();
            };

        }
    }
}
