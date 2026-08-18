#include "D3D11RenderAPI.h"
#include "Debug/DebugLogger.h"

namespace Lindo::Graphics {

    Microsoft::WRL::ComPtr<ID3D11Device> D3D11RenderAPI::s_Device = nullptr;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext> D3D11RenderAPI::s_Context = nullptr;

    void D3D11RenderAPI::Init() {
        LOG_INFO("[Graphics API] : Initializing DirectX 11");

        // Инициализация устройства и контекста DX11 
        // (предполагается, что окно и SwapChain создаются в оконной системе, здесь базовый пример инициализации устройства)
        UINT createDeviceFlags = 0;
#ifdef _DEBUG
        createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

        D3D_FEATURE_LEVEL featureLevels[] = {
            D3D_FEATURE_LEVEL_11_0,
            D3D_FEATURE_LEVEL_10_1
        };
        D3D_FEATURE_LEVEL routerFeatureLevel;

        HRESULT hr = D3D11CreateDevice(
            nullptr,
            D3D_DRIVER_TYPE_HARDWARE,
            nullptr,
            createDeviceFlags,
            featureLevels,
            _countof(featureLevels),
            D3D11_SDK_VERSION,
            &s_Device,
            &routerFeatureLevel,
            &s_Context
        );

        if (FAILED(hr)) {
            LOG_CRITICAL("Failed to create D3D11 Device and Context!");
            throw std::runtime_error("D3D11 init failed");
        }

        LOG_INFO("[GPU API]      : DirectX 11 Initialized Successfully");

        // Установка начальных состояний
        SetDepthTest(true);
        SetCullFace(true, true);
        SetBlending(true);
    }

    void D3D11RenderAPI::SetViewport(int x, int y, int width, int height) {
        D3D11_VIEWPORT vp;
        vp.TopLeftX = static_cast<float>(x);
        vp.TopLeftY = static_cast<float>(y);
        vp.Width = static_cast<float>(width);
        vp.Height = static_cast<float>(height);
        vp.MinDepth = 0.0f;
        vp.MaxDepth = 1.0f;

        if (s_Context) {
            s_Context->RSSetViewports(1, &vp);
        }
    }

    void D3D11RenderAPI::SetClearColor(const glm::vec4& color) {
        m_ClearColor = color;
    }

    void D3D11RenderAPI::Clear(bool colorBuffer, bool depthBuffer) {
        // В DirectX 11 очистка RTV (RenderTargetView) и DSV (DepthStencilView) 
        // обычно делается на уровне кадров через контекст окна, но базовый сброс выглядит так:
        // (Предполагается, что внешне привязаны актуальные View)
    }

    void D3D11RenderAPI::SetDepthTest(bool enabled) {
        m_DepthTestEnabled = enabled;
        UpdateDepthStencilState();
    }

    void D3D11RenderAPI::SetDepthWrite(bool enabled) {
        m_DepthWriteEnabled = enabled;
        UpdateDepthStencilState();
    }

    void D3D11RenderAPI::SetDepthFuncLess() {
        // Настраивается в DepthStencilState (в DirectX по умолчанию D3D11_COMPARISON_LESS)
    }

    void D3D11RenderAPI::SetDepthFuncLEqual() {
        // Настраивается в DepthStencilState при необходимости
    }

    void D3D11RenderAPI::SetCullFace(bool enabled, bool backFace) {
        m_CullFaceEnabled = enabled;
        m_BackFaceCulling = backFace;
        UpdateRasterizerState();
    }

    void D3D11RenderAPI::SetBlending(bool enabled) {
        m_BlendingEnabled = enabled;
        UpdateBlendState();
    }

    void D3D11RenderAPI::SetMultisampling(bool enabled) {
        m_MultisamplingEnabled = enabled;
        UpdateRasterizerState();
    }

    void D3D11RenderAPI::UpdateDepthStencilState() {
        CD3D11_DEPTH_STENCIL_DESC dsDesc(D3D11_DEFAULT);
        dsDesc.DepthEnable = m_DepthTestEnabled ? TRUE : FALSE;
        dsDesc.DepthWriteMask = m_DepthWriteEnabled ? D3D11_DEPTH_WRITE_MASK_ALL : D3D11_DEPTH_WRITE_MASK_ZERO;

        Microsoft::WRL::ComPtr<ID3D11DepthStencilState> pDSState;
        if (SUCCEEDED(s_Device->CreateDepthStencilState(&dsDesc, &pDSState))) {
            s_Context->OMSetDepthStencilState(pDSState.Get(), 0);
        }
    }

    void D3D11RenderAPI::UpdateRasterizerState() {
        CD3D11_RASTERIZER_DESC desc(D3D11_DEFAULT);
        if (!m_CullFaceEnabled) {
            desc.CullMode = D3D11_CULL_NONE;
        }
        else {
            desc.CullMode = m_BackFaceCulling ? D3D11_CULL_BACK : D3D11_CULL_FRONT;
        }
        desc.MultisampleEnable = m_MultisamplingEnabled ? TRUE : FALSE;

        Microsoft::WRL::ComPtr<ID3D11RasterizerState> pRasterizerState;
        if (SUCCEEDED(s_Device->CreateRasterizerState(&desc, &pRasterizerState))) {
            s_Context->RSSetState(pRasterizerState.Get());
        }
    }

    void D3D11RenderAPI::UpdateBlendState() {
        CD3D11_BLEND_DESC blendDesc(D3D11_DEFAULT);
        auto& rt = blendDesc.RenderTarget[0];
        rt.BlendEnable = m_BlendingEnabled ? TRUE : FALSE;
        if (m_BlendingEnabled) {
            rt.SrcBlend = D3D11_BLEND_SRC_ALPHA;
            rt.DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
            rt.BlendOp = D3D11_BLEND_OP_ADD;
            rt.SrcBlendAlpha = D3D11_BLEND_ONE;
            rt.DestBlendAlpha = D3D11_BLEND_ZERO;
            rt.BlendOpAlpha = D3D11_BLEND_OP_ADD;
            rt.RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
        }

        Microsoft::WRL::ComPtr<ID3D11BlendState> pBlendState;
        if (SUCCEEDED(s_Device->CreateBlendState(&blendDesc, &pBlendState))) {
            float blendFactor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
            s_Context->OMSetBlendState(pBlendState.Get(), blendFactor, 0xffffffff);
        }
    }

}