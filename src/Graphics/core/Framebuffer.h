#pragma once
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include "debug/DebugLogger.h"

namespace Lindo {
    namespace Graphics {
        class Framebuffer {
        public:
            Framebuffer(int width, int height);
            ~Framebuffer();

            void bind();
            void unbind();
            void resize(int width, int height);
            uint32_t getTextureID() const { return m_colorAttachment; }
            inline uint32_t getColorAttachmentID() const { return m_colorAttachment; }
            uint32_t getFBOID() const { return m_fbo; }

        private:
            void invalidate();
            uint32_t m_fbo = 0;
            uint32_t m_colorAttachment = 0;
            uint32_t m_depthAttachment = 0;
            int m_width, m_height;
        };
    }
}