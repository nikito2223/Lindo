#include "RenderCommand.h"
#include "Platform/RenderAPI/OpenGL/OpenGLRenderAPI.h"
#include <Debug/DebugLogger.h>

namespace Lindo::Graphics {

    std::unique_ptr<IRenderAPI>& RenderCommand::GetAPIInstance() {
        static std::unique_ptr<IRenderAPI> s_renderAPI = nullptr;

        // ������� ������ ������ �����, ����� � ���� ������� ���������� (��� ����� main)
        if (!s_renderAPI) {
            switch (IRenderAPI::GetAPI()) {
            case GraphicsAPI::OpenGL:
                s_renderAPI = std::make_unique<OpenGLRenderAPI>();
                break;
            case GraphicsAPI::None:
            default:
                LOG_CRITICAL("[RenderCommand] Unknown or None Graphics API selected!");
                throw std::runtime_error("No valid Graphics API selected");
            }
        }
        return s_renderAPI;
    }

}