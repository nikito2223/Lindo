#pragma once
#include "Core/RenderAPI.h"
#include <d3d11.h>
#include <wrl/client.h> // для Microsoft::WRL::ComPtr

namespace Lindo::Graphics {

    class D3D11RenderAPI : public IRenderAPI {
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

        // Дополнительные методы для доступа к контексту/устройству DX11 при необходимости
        static ID3D11Device* GetDevice() { return s_Device.Get(); }
        static ID3D11DeviceContext* GetContext() { return s_Context.Get(); }

    private:
        static Microsoft::WRL::ComPtr<ID3D11Device> s_Device;
        static Microsoft::WRL::ComPtr<ID3D11DeviceContext> s_Context;

        // Внутренние состояния для эмуляции OpenGL-подобного переключения состояний в DX11
        bool m_DepthTestEnabled = true;
        bool m_DepthWriteEnabled = true;
        bool m_CullFaceEnabled = true;
        bool m_BackFaceCulling = true;
        bool m_BlendingEnabled = true;
        bool m_MultisamplingEnabled = true;

        glm::vec4 m_ClearColor = { 0.0f, 0.0f, 0.0f, 1.0f };

        void UpdateDepthStencilState();
        void UpdateRasterizerState();
        void UpdateBlendState();
    };

}