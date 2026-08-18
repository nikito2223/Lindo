#include "RenderCommand.h"
#include "Graphics/platform/OpenGL/OpenGLRenderAPI.h"
#include <Graphics/platform/DirectX11/D3D11RenderAPI.h>
#include <Debug/DebugLogger.h>

namespace Lindo::Graphics {

    std::unique_ptr<IRenderAPI> RenderCommand::s_renderAPI = []() {
        switch (IRenderAPI::GetAPI()) {
        case GraphicsAPI::OpenGL:
            return std::unique_ptr<IRenderAPI>(std::make_unique<OpenGLRenderAPI>());
        case GraphicsAPI::DirectX11:
            return std::unique_ptr<IRenderAPI>(std::make_unique<D3D11RenderAPI>());
        case GraphicsAPI::None:
        default:
            LOG_CRITICAL("Unknown or None Graphics API selected!");
            throw std::runtime_error("No valid Graphics API selected");
        }
        }();
}