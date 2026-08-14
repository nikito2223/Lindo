#pragma once

#include "Core/OGL.h"
#include <glm/glm.hpp>

namespace Lindo {
namespace Graphics {

//==============================================================================
// ShadowMount  - the target a shadow pass renders into.
//
//   - ShadowMount::Array2D : a GL_TEXTURE_2D_ARRAY whose layers are the
//     directional cascades. One FBO, one render call per cascade.
//   - ShadowMount::Cube    : a GL_TEXTURE_CUBE_MAP used for omnidirectional
//     point-light shadows. One FBO, one render call per cube face.
//
// This class owns all GL resources and is fully decoupled from any specific
// light or caster. Copying is forbidden; use std::unique_ptr.
//==============================================================================
class ShadowFramebuffer {
public:
    enum class MountType {
        Array2D,
        Cube,
        Texture2D
    };

    ShadowFramebuffer();
    ~ShadowFramebuffer();

    ShadowFramebuffer(const ShadowFramebuffer&) = delete;
    ShadowFramebuffer& operator=(const ShadowFramebuffer&) = delete;
    ShadowFramebuffer(ShadowFramebuffer&&) noexcept;
    ShadowFramebuffer& operator=(ShadowFramebuffer&&) noexcept;

    // (Re)allocates the depth texture at the given size. For Array2D,
    // 'layers' is the cascade count; for Cube it is ignored (always 6).
    bool init(MountType type, unsigned int size, int layers);

    // Selects the target layer/face and clears the depth buffer.
    void beginRender(int layerOrFace = 0);

    // Binds the whole depth texture to a texture unit for the forward pass.
    void bindForReading(unsigned int textureUnit) const;

    unsigned int textureID() const { return m_texture; }
    unsigned int size() const { return m_size; }
    int layers() const { return m_layers; }
    MountType type() const { return m_type; }
    bool valid() const { return m_valid; }

private:
    void destroy();

    unsigned int m_fbo       = 0;
    unsigned int m_texture   = 0;
    unsigned int m_size      = 0;
    int          m_layers    = 0;
    MountType    m_type      = MountType::Array2D;
    bool         m_valid     = false;
};

} // namespace Graphics
} // namespace Lindo
