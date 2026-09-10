#pragma once
#include "RenderAPI.h"
#include <memory>

namespace Lindo::Graphics {

    class RenderCommand {
    public:
        static void Init() {
            GetAPIInstance()->Init();
        }

        static void SetViewport(int x, int y, int width, int height) {
            GetAPIInstance()->SetViewport(x, y, width, height);
        }

        static void SetClearColor(const glm::vec4& color) {
            GetAPIInstance()->SetClearColor(color);
        }

        static void Clear(bool color = true, bool depth = true) {
            GetAPIInstance()->Clear(color, depth);
        }

        static void SetDepthTest(bool enabled) { GetAPIInstance()->SetDepthTest(enabled); }
        static void SetDepthWrite(bool enabled) { GetAPIInstance()->SetDepthWrite(enabled); }
        static void SetDepthFuncLess() { GetAPIInstance()->SetDepthFuncLess(); }
        static void SetDepthFuncLEqual() { GetAPIInstance()->SetDepthFuncLEqual(); }

        static void SetCullFace(bool enabled, bool backFace = true) { GetAPIInstance()->SetCullFace(enabled, backFace); }
        static void SetBlending(bool enabled) { GetAPIInstance()->SetBlending(enabled); }
        static void SetMultisampling(bool enabled) { GetAPIInstance()->SetMultisampling(enabled); }

    private:
        // Ленивая инициализация: объект создастся только при первом обращении
        static std::unique_ptr<IRenderAPI>& GetAPIInstance();
    };

}