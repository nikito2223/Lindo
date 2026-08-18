#pragma once
#include "Core/RenderAPI.h"

namespace Lindo::Graphics {

    class OpenGLRenderAPI : public IRenderAPI {
    public:
        void Init() override;
        void SetViewport(int x, int y, int width, int height) override;
        void SetClearColor(const glm::vec4& color) override;
        void Clear(bool colorBuffer = true, bool depthBuffer = true) override;

        void SetDepthTest(bool enabled) override;
        void SetDepthWrite(bool enabled) override;
        void SetDepthFuncLess() override;
        void SetDepthFuncLEqual() override;

        void SetCullFace(bool enabled, bool backFace = true) override;
        void SetBlending(bool enabled) override;
        void SetMultisampling(bool enabled) override;
    };

}