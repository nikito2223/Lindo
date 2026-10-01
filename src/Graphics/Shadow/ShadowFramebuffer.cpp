#include "ShadowFramebuffer.h"
#include "Graphics/core/Shader.h"
#include "Debug/DebugLogger.h"
#include "Core/RenderCommand.h"

namespace Lindo {
    namespace Graphics {

        ShadowFramebuffer::ShadowFramebuffer() = default;

        ShadowFramebuffer::~ShadowFramebuffer() {
            destroy();
        }

        ShadowFramebuffer::ShadowFramebuffer(ShadowFramebuffer&& other) noexcept
            : m_fbo(other.m_fbo),
            m_texture(other.m_texture),
            m_size(other.m_size),
            m_layers(other.m_layers),
            m_type(other.m_type),
            m_valid(other.m_valid) {
            other.m_fbo = 0;
            other.m_texture = 0;
            other.m_size = 0;
            other.m_layers = 0;
            other.m_valid = false;
        }

        ShadowFramebuffer& ShadowFramebuffer::operator=(ShadowFramebuffer&& other) noexcept {
            if (this != &other) {
                destroy();
                m_fbo = other.m_fbo;
                m_texture = other.m_texture;
                m_size = other.m_size;
                m_layers = other.m_layers;
                m_type = other.m_type;
                m_valid = other.m_valid;
                other.m_fbo = 0;
                other.m_texture = 0;
                other.m_size = 0;
                other.m_layers = 0;
                other.m_valid = false;
            }
            return *this;
        }

        void ShadowFramebuffer::destroy() {
            if (m_fbo)     glDeleteFramebuffers(1, &m_fbo);
            if (m_texture) glDeleteTextures(1, &m_texture);
            m_fbo = 0;
            m_texture = 0;
            m_size = 0;
            m_layers = 0;
            m_valid = false;
        }

        bool ShadowFramebuffer::init(MountType type, unsigned int size, int layers) {
            destroy();

            m_type = type;
            GLint maxViewportDims[2] = { 0, 0 };
            glGetIntegerv(GL_MAX_VIEWPORT_DIMS, maxViewportDims);
            const GLint maxShadowViewport = glm::min(maxViewportDims[0], maxViewportDims[1]);
            m_size = maxShadowViewport > 0
                ? glm::min(size, static_cast<unsigned int>(maxShadowViewport))
                : size;
            if (m_size < size) {
                LOG_WARN("ShadowFramebuffer::init - shadow map size reduced from " +
                    std::to_string(size) + " to " + std::to_string(m_size) +
                    " to fit the device viewport limit.");
            }
            m_layers = (type == MountType::Cube) ? 6 : glm::max(layers, 1);

            glGenFramebuffers(1, &m_fbo);
            glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);

            if (type == MountType::Array2D) {
                glGenTextures(1, &m_texture);
                glBindTexture(GL_TEXTURE_2D_ARRAY, m_texture);

                glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_DEPTH_COMPONENT32F,
                    static_cast<GLsizei>(m_size), static_cast<GLsizei>(m_size),
                    m_layers, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);

                glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
                glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
                const float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
                glTexParameterfv(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_BORDER_COLOR, borderColor);
                glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_COMPARE_MODE, GL_NONE);

                glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, m_texture, 0);
            }
            else if (type == MountType::Texture2D) {
                glGenTextures(1, &m_texture);
                glBindTexture(GL_TEXTURE_2D, m_texture);

                glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F,
                    static_cast<GLsizei>(m_size), static_cast<GLsizei>(m_size),
                    0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);

                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
                const float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
                glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_NONE);

                glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_texture, 0);
            }
            else {
                glGenTextures(1, &m_texture);
                glBindTexture(GL_TEXTURE_CUBE_MAP, m_texture);

                for (unsigned int i = 0; i < 6; ++i) {
                    glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_DEPTH_COMPONENT32F,
                        static_cast<GLsizei>(m_size), static_cast<GLsizei>(m_size),
                        0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
                }

                glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
                glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
                glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_COMPARE_MODE, GL_NONE);

                glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, m_texture, 0);
            }

            glDrawBuffer(GL_NONE);
            glReadBuffer(GL_NONE);

            const GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
            Shader::logOpenGLErrors("shadow framebuffer creation");
            glBindFramebuffer(GL_FRAMEBUFFER, 0);

            if (status != GL_FRAMEBUFFER_COMPLETE) {
                LOG_ERROR("ShadowFramebuffer::init - FBO incomplete (status=" +
                    std::to_string(static_cast<int>(status)) + ")");
                destroy();
                return false;
            }

            m_valid = true;
            return true;
        }

        void ShadowFramebuffer::beginRender(int layerOrFace) {
            if (!m_valid) return;

            glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
            Shader::logOpenGLErrors("shadow framebuffer bind");
            RenderCommand::SetViewport(0, 0, static_cast<int>(m_size), static_cast<int>(m_size));
            Shader::logOpenGLErrors("shadow framebuffer viewport");

            if (m_type == MountType::Array2D) {
                glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, m_texture, 0, layerOrFace);
            }
            else if (m_type == MountType::Texture2D) {
                glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_texture, 0);
            }
            else {
                const int face = glm::clamp(layerOrFace, 0, 5);
                glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                    GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, m_texture, 0);
            }
            Shader::logOpenGLErrors("shadow framebuffer attachment");

            const GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
            if (status != GL_FRAMEBUFFER_COMPLETE) {
                LOG_ERROR("ShadowFramebuffer::beginRender - FBO incomplete (status=" +
                    std::to_string(static_cast<int>(status)) + ", layer=" +
                    std::to_string(layerOrFace) + ")");
            }
            Shader::logOpenGLErrors("shadow framebuffer status check");

            RenderCommand::Clear(false, true);
        }

        void ShadowFramebuffer::bindForReading(unsigned int textureUnit) const {
            if (!m_valid) return;
            glActiveTexture(GL_TEXTURE0 + textureUnit);
            GLenum target = GL_TEXTURE_CUBE_MAP;
            if (m_type == MountType::Array2D) target = GL_TEXTURE_2D_ARRAY;
            else if (m_type == MountType::Texture2D) target = GL_TEXTURE_2D;
            glBindTexture(target, m_texture);
        }

    } // namespace Graphics
} // namespace Lindo