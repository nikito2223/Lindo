#pragma once
#include <glm/glm.hpp>

namespace Lindo::Graphics {

    enum class GraphicsAPI {
        None = 0,
        OpenGL = 1,
        DirectX11 = 2, // На будущее
        Vulkan = 3     // На будущее
    };

    class IRenderAPI {
    public:
        virtual ~IRenderAPI() = default;

        virtual void Init() = 0;
        virtual void SetViewport(int x, int y, int width, int height) = 0;
        virtual void SetClearColor(const glm::vec4& color) = 0;
        virtual void Clear(bool colorBuffer = true, bool depthBuffer = true) = 0;

        // Управление состояниями (State Management)
        virtual void SetDepthTest(bool enabled) = 0;
        virtual void SetDepthWrite(bool enabled) = 0;
        virtual void SetDepthFuncLess() = 0;
        virtual void SetDepthFuncLEqual() = 0;

        virtual void SetCullFace(bool enabled, bool backFace = true) = 0;
        virtual void SetBlending(bool enabled) = 0;
        virtual void SetMultisampling(bool enabled) = 0;

        static void SetAPI(GraphicsAPI api) { s_API = api; }
        static GraphicsAPI GetAPI() { return s_API; }

    private:
        static GraphicsAPI s_API;
    };

}