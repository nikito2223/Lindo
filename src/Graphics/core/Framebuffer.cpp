#include "Framebuffer.h"
#include "Debug/DebugLogger.h"

namespace Lindo {
    namespace Graphics {
        namespace {
            const char* FramebufferStatusName(GLenum status) {
                switch (status) {
                case GL_FRAMEBUFFER_COMPLETE: return "GL_FRAMEBUFFER_COMPLETE";
                case GL_FRAMEBUFFER_UNDEFINED: return "GL_FRAMEBUFFER_UNDEFINED";
                case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT: return "GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT";
                case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT: return "GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT";
                case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER: return "GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER";
                case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER: return "GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER";
                case GL_FRAMEBUFFER_UNSUPPORTED: return "GL_FRAMEBUFFER_UNSUPPORTED";
                case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE: return "GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE";
                default: return "unknown framebuffer status";
                }
            }
        }

        Framebuffer::Framebuffer(int width, int height) : m_width(width), m_height(height) {
            invalidate();
        }

        Framebuffer::~Framebuffer() {
            glDeleteFramebuffers(1, &m_fbo);
            glDeleteTextures(1, &m_colorAttachment);
            glDeleteRenderbuffers(1, &m_depthAttachment);
        }

        void Framebuffer::invalidate() {
            if (m_fbo) {
                glDeleteFramebuffers(1, &m_fbo);
                glDeleteTextures(1, &m_colorAttachment);
                glDeleteRenderbuffers(1, &m_depthAttachment);
            }
            glGenFramebuffers(1, &m_fbo);
            glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);

            // Color attachment
            glGenTextures(1, &m_colorAttachment);
            glBindTexture(GL_TEXTURE_2D, m_colorAttachment);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, m_width, m_height, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_colorAttachment, 0);

            // Depth & Stencil attachment
            glGenRenderbuffers(1, &m_depthAttachment);
            glBindRenderbuffer(GL_RENDERBUFFER, m_depthAttachment);
            glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, m_width, m_height);
            glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, m_depthAttachment);

            m_status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
            if (m_status != GL_FRAMEBUFFER_COMPLETE) {
                LOG_ERROR("[Framebuffer] FBO " + std::to_string(m_fbo) + " is incomplete: " +
                    FramebufferStatusName(m_status) + " (0x" + std::to_string(static_cast<unsigned int>(m_status)) +
                    "), size=" + std::to_string(m_width) + "x" + std::to_string(m_height) + ".");
            }
            else {
                LOG_INFO("[Framebuffer] FBO " + std::to_string(m_fbo) + " complete: " +
                    std::to_string(m_width) + "x" + std::to_string(m_height) + ".");
            }

            glBindFramebuffer(GL_FRAMEBUFFER, 0);
        }

        void Framebuffer::bind() {
            glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
            glViewport(0, 0, m_width, m_height);
        }

        void Framebuffer::unbind() {
            glBindFramebuffer(GL_FRAMEBUFFER, 0);

            // ROOT CAUSE FIX:
            // bind() sets glViewport to this FBO's own (possibly stale) width/height.
            // Previously unbind() only rebound the default framebuffer and left that
            // viewport in place. If the window/monitor size had since changed
            // (resolution change, entering fullscreen) but this FBO hadn't been
            // resized yet in the same frame, the final on-screen pass kept
            // rendering into the OLD, smaller rectangle - leaving the new window
            // area outside it uncovered, which shows up as grey bars (the GL clear
            // color) along the edges.
            //
            // Query the actual current window's framebuffer size directly from
            // GLFW and restore the viewport to match it, so the default
            // framebuffer always fills the real window regardless of whether this
            // FBO has been resized yet.
            GLFWwindow* currentContext = glfwGetCurrentContext();
            if (currentContext) {
                int screenWidth = 0, screenHeight = 0;
                glfwGetFramebufferSize(currentContext, &screenWidth, &screenHeight);
                if (screenWidth > 0 && screenHeight > 0) {
                    glViewport(0, 0, screenWidth, screenHeight);
                }
            }
        }

        void Framebuffer::resize(int width, int height) {
            if (m_width == width && m_height == height) return;
            m_width = width;
            m_height = height;
            invalidate();
        }
    }
}