#pragma once

#include <glm/glm.hpp>
#include <Component/Component.h>
#include <Component/GameObject/GameObject.h>
#include <algorithm>

namespace Lindo {
    namespace Components {
        namespace Physics {

            // A configurable gravity/force field. Supports three modes:
            //   - Global: a constant acceleration applied everywhere.
            //   - Directional: a constant acceleration along a fixed direction.
            //   - Radial: attraction toward (or away from) a focal point.
            //
            // The field can be attached to a GameObject and its influence is
            // queried by the PhysicsSystem when integrating rigid bodies.
            class GravityField : public World::Component {
            public:
                enum class FieldType {
                    Global,
                    Directional,
                    Radial
                };

                GravityField() = default;

                // ----- Configuration -----
                void SetType(FieldType type) { fieldType = type; }
                FieldType GetType() const { return fieldType; }

                void SetGlobalGravity(const glm::vec3& gravity) { globalGravity = gravity; }
                glm::vec3 GetGlobalGravity() const { return globalGravity; }

                void SetDirection(const glm::vec3& dir) { direction = glm::normalize(dir); }
                glm::vec3 GetDirection() const { return direction; }

void SetStrength(float newStrength) { strength = newStrength; }
                float GetStrength() const { return strength; }

                void SetFocalPoint(const glm::vec3& focal) { focalPoint = focal; }
                void SetFocalPointFromgameObject() { if (gameObject) focalPoint = gameObject->transform.position; }
                glm::vec3 GetFocalPoint() const { return focalPoint; }

void SetRadius(float newRadius) { radius = std::max(0.0f, newRadius); }
                float GetRadius() const { return radius; }

                void SetEnabled(bool enabled) { isEnabled = enabled; }
                bool IsEnabled() const { return isEnabled; }

                // Computes the acceleration at a given world-space point.
                // Returns the acceleration vector (m/s^2).
                glm::vec3 GetAccelerationAt(const glm::vec3& worldPoint) const;

                // Global convenience: the default global gravity for the scene.
                static glm::vec3 s_globalGravity;

            private:
                FieldType fieldType = FieldType::Global;
                glm::vec3 globalGravity{ 0.0f, -9.81f, 0.0f };
                glm::vec3 direction{ 0.0f, -1.0f, 0.0f };
                float strength = 9.81f;
                glm::vec3 focalPoint{ 0.0f };
                float radius = 100.0f;
                bool isEnabled = true;
            };

        }
    }
}
