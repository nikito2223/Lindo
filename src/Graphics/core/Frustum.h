#pragma once

#include <glm/glm.hpp>
#include <Physics/Math/AABB.h>

namespace Lindo::Graphics {

    class Frustum {
    public:
        void update(const glm::mat4& viewProjection);
        bool intersects(const Lindo::Math::AABB& bounds) const;

    private:
        glm::vec4 m_planes[6]{};
    };
}
