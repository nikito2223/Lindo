#pragma once
#include "Core/OGL.h"
#include <glm/glm.hpp>
#include <vector>
#include <cstdint>

namespace Lindo {
    namespace Graphics {

        enum class FramebufferTextureFormat {
            None = 0,
            
            // Color
            RGBA8,
            RGBA16F,
            RGBA32F,
            RED_INTEGER,

            // Depth/Stencil
            DEPTH24STENCIL8,
            DEPTH32F,

            // Defaults
            Depth = DEPTH24STENCIL8
        };

        struct FramebufferTextureSpecification {
            FramebufferTextureSpecification() = default;
            FramebufferTextureSpecification(FramebufferTextureFormat format)
                : textureFormat(format) {}

            FramebufferTextureFormat textureFormat = FramebufferTextureFormat::None;
            // Можно добавить фильтрацию (Linear/Nearest), Wrap-режимы и т.д.
        };

        struct FramebufferAttachmentSpecification {
            FramebufferAttachmentSpecification() = default;
            FramebufferAttachmentSpecification(std::initializer_list<FramebufferTextureSpecification> attachments)
                : attachments(attachments) {}

            std::vector<FramebufferTextureSpecification> attachments;
        };

        struct FramebufferSpecification {
            uint32_t width = 0;
            uint32_t height = 0;
            uint32_t samples = 1; // Для MSAA (в будущем)
            FramebufferAttachmentSpecification attachments;
            bool swapChainTarget = false;
        };

        class Framebuffer {
        public:
            explicit Framebuffer(const FramebufferSpecification& spec);
            ~Framebuffer();

            Framebuffer(const Framebuffer&) = delete;
            Framebuffer& operator=(const Framebuffer&) = delete;

            Framebuffer(Framebuffer&& other) noexcept;
            Framebuffer& operator=(Framebuffer&& other) noexcept;

            void bind() const;
            void unbind() const;

            void resize(uint32_t width, uint32_t height);

            uint32_t getRendererID() const { return m_rendererID; }
            uint32_t getColorAttachmentRendererID(uint32_t index = 0) const;
            uint32_t getDepthAttachmentRendererID() const { return m_depthAttachment; }

            const FramebufferSpecification& getSpecification() const { return m_specification; }

            // Чтение пикселей (полезно для Picking / выбора объектов мышью в эдиторе)
            int readPixel(uint32_t attachmentIndex, int x, int y);
            void clearAttachment(uint32_t attachmentIndex, int value);

        private:
            void invalidate();
            void release();

        private:
            uint32_t m_rendererID = 0;
            FramebufferSpecification m_specification;

            std::vector<FramebufferTextureSpecification> m_colorAttachmentSpecs;
            FramebufferTextureSpecification m_depthAttachmentSpec = FramebufferTextureFormat::None;

            std::vector<uint32_t> m_colorAttachments;
            uint32_t m_depthAttachment = 0;
        };

    }
}