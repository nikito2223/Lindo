#pragma once

#include <glm/glm.hpp>
#include <memory>
#include <Component/Component.h>

namespace Lindo {
    namespace World {
        class GameObject;
        class Component;
    }
    namespace Components {
        namespace Physics {
            class Collider;
        }
    }
}

namespace Lindo {
    namespace Components {
        namespace Physics {

            // A configurable rigid body component. Handles linear and angular
            // dynamics, gravity (global + per-body scale), forces, impulses,
            // and damping. It can be attached to any GameObject alongside a
            // collider to produce physically simulated motion.
            class RigidBody : public Lindo::World::Component {
            public:
                RigidBody(float mass = 1.0f);

                void OnStart() override;
                void OnDestroy() override;

                void OnUpdate(float deltaTime, const glm::vec3& gravity);
                void OnUpdate(float deltaTime) override;

                // ----- Force / impulse API -----
                void applyForce(const glm::vec3& force);
                void applyForceAtPoint(const glm::vec3& force, const glm::vec3& worldPoint);
                void applyImpulse(const glm::vec3& impulse);
                void applyImpulseAtPoint(const glm::vec3& impulse, const glm::vec3& worldPoint);
                void applyTorque(const glm::vec3& torque);
                void clearForces();

                // ----- Integration -----
                void integrate(float deltaTime, const glm::vec3& gravity);

                // ----- Configuration (fully exposed & configurable) -----
                float mass = 1.0f;
                float invMass = 1.0f;
                glm::vec3 velocity{ 0.0f };
                glm::vec3 angularVelocity{ 0.0f };
                glm::vec3 acceleration{ 0.0f };

                bool useGravity = true;
                float gravityScale = 1.0f;

                float restitution = 0.2f;   // bounciness [0,1]
                float friction = 0.5f;      // surface friction [0,1]
                float linearDamping = 0.002f;
                float angularDamping = 0.01f;

                bool isKinematic = false;   // kinematic bodies are moved externally
                bool isSleeping = false;
                float sleepThreshold = 0.05f;

                // Reference to the attached collider (for collision response).
                Lindo::Components::Physics::Collider* collider = nullptr;

                bool isGrounded = false;

                // ----- State helpers -----
                void SetMass(float newMass);
                float GetMass() const { return mass; }
                float GetInvMass() const { return invMass; }
                const glm::vec3& GetVelocity() const { return velocity; }
                void SetVelocity(const glm::vec3& v) { velocity = v; }
                void SetAngularVelocity(const glm::vec3& w) { angularVelocity = w; }
                void SetKinematic(bool kinematic) { isKinematic = kinematic; }
                bool IsKinematic() const { return isKinematic; }
                void SetGravityScale(float scale) { gravityScale = scale; }
                void SetUseGravity(bool enabled) { useGravity = enabled; }
                void Wake() { isSleeping = false; }

                // Inertia tensor (for a simple box/sphere approximation).
                // Used in angular impulse response.
                glm::mat3 GetInertiaTensor() const;

            private:
                void ClearAccumulators();
                void ApplyDamping(float deltaTime);
                void ClampVelocity(float maxSpeed = 25.0f);
                void UpdateSleepState(float deltaTime);

                // Accumulators cleared each step.
                glm::vec3 forceAccumulator{ 0.0f };
                glm::vec3 torqueAccumulator{ 0.0f };
                float sleepTimer = 0.0f;
            };

        }
    }
}
