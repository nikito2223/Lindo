#include "Frustum.h"

#include <glm/gtc/type_ptr.hpp>

namespace Lindo::Graphics {

    void Frustum::update(const glm::mat4& matrix) {
        const glm::mat4 transposed = glm::transpose(matrix);
        m_planes[0] = transposed[3] + transposed[0];
        m_planes[1] = transposed[3] - transposed[0];
        m_planes[2] = transposed[3] + transposed[1];
        m_planes[3] = transposed[3] - transposed[1];
        m_planes[4] = transposed[3] + transposed[2];
        m_planes[5] = transposed[3] - transposed[2];

        for (glm::vec4& plane : m_planes) {
            const float length = glm::length(glm::vec3(plane));
            if (length > 0.0f) plane /= length;
        }
    }

    bool Frustum::intersects(const Lindo::Math::AABB& bounds) const {
        if (!bounds.isValid()) return true;

        for (const glm::vec4& plane : m_planes) {
            const glm::vec3 positive(
                plane.x >= 0.0f ? bounds.max.x : bounds.min.x,
                plane.y >= 0.0f ? bounds.max.y : bounds.min.y,
                plane.z >= 0.0f ? bounds.max.z : bounds.min.z);
            if (glm::dot(glm::vec3(plane), positive) + plane.w < 0.0f) return false;
        }
        return true;
    }
}
