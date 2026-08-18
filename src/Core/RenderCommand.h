#pragma once
#include "RenderAPI.h"
#include <memory>

namespace Lindo::Graphics {

    class RenderCommand {
    public:
        static void Init() {
            s_renderAPI->Init();
        }

        static void SetViewport(int x, int y, int width, int height) {
            s_renderAPI->SetViewport(x, y, width, height);
        }

        static void SetClearColor(const glm::vec4& color) {
            s_renderAPI->SetClearColor(color);
        }

        static void Clear(bool color = true, bool depth = true) {
            s_renderAPI->Clear(color, depth);
        }

        static void SetDepthTest(bool enabled) { s_renderAPI->SetDepthTest(enabled); }
        static void SetDepthWrite(bool enabled) { s_renderAPI->SetDepthWrite(enabled); }
        static void SetDepthFuncLess() { s_renderAPI->SetDepthFuncLess(); }
        static void SetDepthFuncLEqual() { s_renderAPI->SetDepthFuncLEqual(); }

        static void SetCullFace(bool enabled, bool backFace = true) { s_renderAPI->SetCullFace(enabled, backFace); }
        static void SetBlending(bool enabled) { s_renderAPI->SetBlending(enabled); }
        static void SetMultisampling(bool enabled) { s_renderAPI->SetMultisampling(enabled); }

    private:
        static std::unique_ptr<IRenderAPI> s_renderAPI;
    };

}