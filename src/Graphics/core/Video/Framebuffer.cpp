#include "Framebuffer.h"
#include "Debug/DebugLogger.h"
#include <iostream>

namespace Lindo {
    namespace Graphics {

        static const uint32_t s_maxFramebufferSize = 8192;

        static bool isDepthFormat(FramebufferTextureFormat format) {
            switch (format) {
                case FramebufferTextureFormat::DEPTH24STENCIL8:
                case FramebufferTextureFormat::DEPTH32F:
                    return true;
                default:
                    return false;
            }
        }

        static GLenum lindoTextureFormatToGL(FramebufferTextureFormat format) {
            switch (format) {
                case FramebufferTextureFormat::RGBA8:       return GL_RGBA8;
                case FramebufferTextureFormat::RGBA16F:     return GL_RGBA16F;
                case FramebufferTextureFormat::RGBA32F:     return GL_RGBA32F;
                case FramebufferTextureFormat::RED_INTEGER: return GL_R32I;
                default: return 0;
            }
        }

        Framebuffer::Framebuffer(const FramebufferSpecification& spec)
            : m_specification(spec) {
            for (auto attachment : m_specification.attachments.attachments) {
                if (!isDepthFormat(attachment.textureFormat))
                    m_colorAttachmentSpecs.emplace_back(attachment);
                else
                    m_depthAttachmentSpec = attachment;
            }

            invalidate();
        }

        Framebuffer::~Framebuffer() {
            release();
        }

        Framebuffer::Framebuffer(Framebuffer&& other) noexcept {
            *this = std::move(other);
        }

        Framebuffer& Framebuffer::operator=(Framebuffer&& other) noexcept {
            if (this != &other) {
                release();

                m_rendererID = other.m_rendererID;
                m_specification = other.m_specification;
                m_colorAttachmentSpecs = std::move(other.m_colorAttachmentSpecs);
                m_depthAttachmentSpec = other.m_depthAttachmentSpec;
                m_colorAttachments = std::move(other.m_colorAttachments);
                m_depthAttachment = other.m_depthAttachment;

                other.m_rendererID = 0;
                other.m_depthAttachment = 0;
            }
            return *this;
        }

        void Framebuffer::release() {
            if (m_rendererID) {
                glDeleteFramebuffers(1, &m_rendererID);
                glDeleteTextures(static_cast<GLsizei>(m_colorAttachments.size()), m_colorAttachments.data());
                glDeleteTextures(1, &m_depthAttachment);

                m_colorAttachments.clear();
                m_depthAttachment = 0;
                m_rendererID = 0;
            }
        }

        void Framebuffer::invalidate() {
            if (m_rendererID) {
                release();
            }

            glGenFramebuffers(1, &m_rendererID);
            glBindFramebuffer(GL_FRAMEBUFFER, m_rendererID);

            bool multisample = m_specification.samples > 1;

            // --- Color Attachments ---
            if (!m_colorAttachmentSpecs.empty()) {
                m_colorAttachments.resize(m_colorAttachmentSpecs.size());
                glGenTextures(static_cast<GLsizei>(m_colorAttachments.size()), m_colorAttachments.data());

                for (size_t i = 0; i < m_colorAttachments.size(); i++) {
                    glBindTexture(GL_TEXTURE_2D, m_colorAttachments[i]);

                    switch (m_colorAttachmentSpecs[i].textureFormat) {
                        case FramebufferTextureFormat::RGBA8:
                            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, m_specification.width, m_specification.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
                            break;
                        case FramebufferTextureFormat::RGBA16F:
                            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, m_specification.width, m_specification.height, 0, GL_RGBA, GL_FLOAT, nullptr);
                            break;
                        case FramebufferTextureFormat::RGBA32F:
                            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, m_specification.width, m_specification.height, 0, GL_RGBA, GL_FLOAT, nullptr);
                            break;
                        case FramebufferTextureFormat::RED_INTEGER:
                            glTexImage2D(GL_TEXTURE_2D, 0, GL_R32I, m_specification.width, m_specification.height, 0, GL_RED_INTEGER, GL_INT, nullptr);
                            break;
                        default:
                            break;
                    }

                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

                    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + static_cast<GLenum>(i), GL_TEXTURE_2D, m_colorAttachments[i], 0);
                }
            }

            // --- Depth Attachment ---
            if (m_depthAttachmentSpec.textureFormat != FramebufferTextureFormat::None) {
                glGenTextures(1, &m_depthAttachment);
                glBindTexture(GL_TEXTURE_2D, m_depthAttachment);

                switch (m_depthAttachmentSpec.textureFormat) {
                    case FramebufferTextureFormat::DEPTH24STENCIL8:
                        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, m_specification.width, m_specification.height, 0, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, nullptr);
                        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, m_depthAttachment, 0);
                        break;
                    case FramebufferTextureFormat::DEPTH32F:
                        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, m_specification.width, m_specification.height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
                        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_depthAttachment, 0);
                        break;
                    default:
                        break;
                }

                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            }

            // Draw Buffers Setup (для MRT)
            if (m_colorAttachments.size() > 1) {
                std::vector<GLenum> buffers(m_colorAttachments.size());
                for (size_t i = 0; i < buffers.size(); ++i) {
                    buffers[i] = GL_COLOR_ATTACHMENT0 + static_cast<GLenum>(i);
                }
                glDrawBuffers(static_cast<GLsizei>(buffers.size()), buffers.data());
            } else if (m_colorAttachments.empty()) {
                glDrawBuffer(GL_NONE);
                glReadBuffer(GL_NONE);
            }

            if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
                LOG_CRITICAL("Framebuffer is incomplete!");
            }

            glBindFramebuffer(GL_FRAMEBUFFER, 0);
        }

        void Framebuffer::bind() const {
            glBindFramebuffer(GL_FRAMEBUFFER, m_rendererID);
            glViewport(0, 0, m_specification.width, m_specification.height);
        }

        void Framebuffer::unbind() const {
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
        }

        void Framebuffer::resize(uint32_t width, uint32_t height) {
            if (width == 0 || height == 0 || width > s_maxFramebufferSize || height > s_maxFramebufferSize) {
                LOG_WARN("Attempted to resize framebuffer to invalid size: " + std::to_string(width) + "x" + std::to_string(height));
                return;
            }

            m_specification.width = width;
            m_specification.height = height;
            invalidate();
        }

        uint32_t Framebuffer::getColorAttachmentRendererID(uint32_t index) const {
            if (index < m_colorAttachments.size()) {
                return m_colorAttachments[index];
            }
            return 0;
        }

        int Framebuffer::readPixel(uint32_t attachmentIndex, int x, int y) {
            if (attachmentIndex >= m_colorAttachments.size()) return -1;

            glReadBuffer(GL_COLOR_ATTACHMENT0 + attachmentIndex);
            int pixelData;
            glReadPixels(x, y, 1, 1, GL_RED_INTEGER, GL_INT, &pixelData);
            return pixelData;
        }

        void Framebuffer::clearAttachment(uint32_t attachmentIndex, int value) {
            if (attachmentIndex >= m_colorAttachments.size()) return;

            auto& spec = m_colorAttachmentSpecs[attachmentIndex];
            glClearBufferiv(GL_COLOR, attachmentIndex, &value);
        }

    }
}