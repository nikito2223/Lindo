#include "GravityField.h"
#include <cmath>

namespace Lindo {
    namespace Components {
        namespace Physics {

            glm::vec3 GravityField::s_globalGravity(0.0f, -9.81f, 0.0f);

            glm::vec3 GravityField::GetAccelerationAt(const glm::vec3& worldPoint) const {
                if (!isEnabled) {
                    return glm::vec3(0.0f);
                }

                switch (fieldType) {
                case FieldType::Global:
                    return globalGravity;

                case FieldType::Directional:
                    return direction * strength;

                case FieldType::Radial: {
                    glm::vec3 toFocal = focalPoint - worldPoint;
                    float dist = glm::length(toFocal);
                    if (dist < 1e-6f) {
                        return glm::vec3(0.0f);
                    }
                    // Attract toward focal point with 1/r^2 falloff, clamped
                    // to avoid singularities near the center.
                    float clampedDist = std::max(dist, 0.1f);
                    float magnitude = strength / (clampedDist * clampedDist);
                    return (toFocal / dist) * magnitude;
                }

                default:
                    return globalGravity;
                }
            }

        }
    }
}
