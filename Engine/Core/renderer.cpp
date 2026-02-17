#include "renderer.h"
#include <d3dcompiler.h>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "d3dcompiler.lib")

namespace Engine {
    Renderer& Renderer::GetInstance() {
        static Renderer instance;
        return instance;
    }

    bool Renderer::Initialize(HINSTANCE hInstance, HWND hWnd, bool windowed) {
        HRESULT hr = S_OK;

        // スワップチェーン設定
        DXGI_SWAP_CHAIN_DESC sd = {};
        sd.BufferCount = 1;
        sd.BufferDesc.Width = m_screenWidth;
        sd.BufferDesc.Height = m_screenHeight;
        sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        sd.BufferDesc.RefreshRate.Numerator = 60;
        sd.BufferDesc.RefreshRate.Denominator = 1;
        sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        sd.OutputWindow = hWnd;
        sd.SampleDesc.Count = 1;
        sd.SampleDesc.Quality = 0;
        sd.Windowed = windowed ? TRUE : FALSE;

        D3D_FEATURE_LEVEL featureLevels[] = {
            D3D_FEATURE_LEVEL_11_0,
            D3D_FEATURE_LEVEL_10_1,
            D3D_FEATURE_LEVEL_10_0,
        };

        D3D_FEATURE_LEVEL featureLevel;

        hr = D3D11CreateDeviceAndSwapChain(
            nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0,
            featureLevels, ARRAYSIZE(featureLevels),
            D3D11_SDK_VERSION, &sd,
            &m_pSwapChain, &m_pDevice, &featureLevel, &m_pContext
        );

        if (FAILED(hr)) {
            // WARPにフォールバック
            hr = D3D11CreateDeviceAndSwapChain(
                nullptr, D3D_DRIVER_TYPE_WARP, nullptr, 0,
                featureLevels, ARRAYSIZE(featureLevels),
                D3D11_SDK_VERSION, &sd,
                &m_pSwapChain, &m_pDevice, &featureLevel, &m_pContext
            );
            if (FAILED(hr)) return false;
        }

        // レンダーターゲットビュー作成
        ID3D11Texture2D* pBackBuffer = nullptr;
        m_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);
        m_pDevice->CreateRenderTargetView(pBackBuffer, nullptr, &m_pRenderTargetView);
        pBackBuffer->Release();

        // 深度ステンシルビュー作成
        D3D11_TEXTURE2D_DESC td = {};
        td.Width = m_screenWidth;
        td.Height = m_screenHeight;
        td.MipLevels = 1;
        td.ArraySize = 1;
        td.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
        td.SampleDesc.Count = 1;
        td.Usage = D3D11_USAGE_DEFAULT;
        td.BindFlags = D3D11_BIND_DEPTH_STENCIL;

        ID3D11Texture2D* pDepthTexture = nullptr;
        m_pDevice->CreateTexture2D(&td, nullptr, &pDepthTexture);

        D3D11_DEPTH_STENCIL_VIEW_DESC dsvd = {};
        dsvd.Format = td.Format;
        dsvd.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
        m_pDevice->CreateDepthStencilView(pDepthTexture, &dsvd, &m_pDepthStencilView);
        pDepthTexture->Release();

        m_pContext->OMSetRenderTargets(1, &m_pRenderTargetView, m_pDepthStencilView);

        // ビューポート設定
        D3D11_VIEWPORT vp = {};
        vp.Width = static_cast<float>(m_screenWidth);
        vp.Height = static_cast<float>(m_screenHeight);
        vp.MinDepth = 0.0f;
        vp.MaxDepth = 1.0f;
        m_pContext->RSSetViewports(1, &vp);

        // ラスタライザステート
        D3D11_RASTERIZER_DESC rd = {};
        rd.FillMode = D3D11_FILL_SOLID;
        rd.CullMode = D3D11_CULL_BACK;
        rd.DepthClipEnable = TRUE;

        ID3D11RasterizerState* pRasterizerState = nullptr;
        m_pDevice->CreateRasterizerState(&rd, &pRasterizerState);
        m_pContext->RSSetState(pRasterizerState);
        pRasterizerState->Release();

        // ブレンドステート
        D3D11_BLEND_DESC blendDesc = {};
        blendDesc.RenderTarget[0].BlendEnable = TRUE;
        blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
        blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
        blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
        blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
        blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
        blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
        blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

        ID3D11BlendState* pBlendState = nullptr;
        m_pDevice->CreateBlendState(&blendDesc, &pBlendState);
        float blendFactor[4] = { 0, 0, 0, 0 };
        m_pContext->OMSetBlendState(pBlendState, blendFactor, 0xffffffff);
        pBlendState->Release();

        // 深度ステンシルステート
        D3D11_DEPTH_STENCIL_DESC dsDesc = {};
        dsDesc.DepthEnable = TRUE;
        dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
        dsDesc.DepthFunc = D3D11_COMPARISON_LESS;
        m_pDevice->CreateDepthStencilState(&dsDesc, &m_pDepthStateEnable);

        dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
        m_pDevice->CreateDepthStencilState(&dsDesc, &m_pDepthStateDisable);

        m_pContext->OMSetDepthStencilState(m_pDepthStateEnable, 0);

        // サンプラーステート
        D3D11_SAMPLER_DESC samplerDesc = {};
        samplerDesc.Filter = D3D11_FILTER_ANISOTROPIC;
        samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
        samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
        samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
        samplerDesc.MaxAnisotropy = 16;
        samplerDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
        samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

        ID3D11SamplerState* pSamplerState = nullptr;
        m_pDevice->CreateSamplerState(&samplerDesc, &pSamplerState);
        m_pContext->PSSetSamplers(0, 1, &pSamplerState);
        pSamplerState->Release();

        // シェーダーコンパイル
        ID3DBlob* pVSBlob = nullptr;
        ID3DBlob* pErrorBlob = nullptr;

        hr = D3DCompileFromFile(L"shader.hlsl", nullptr, nullptr, "VertexShaderPolygon", "vs_4_0_level_9_3",
            D3DCOMPILE_ENABLE_STRICTNESS, 0, &pVSBlob, &pErrorBlob);
        if (FAILED(hr)) {
            if (pErrorBlob) {
                MessageBoxA(nullptr, (char*)pErrorBlob->GetBufferPointer(), "VS Error", MB_OK);
                pErrorBlob->Release();
            }
            return false;
        }

        m_pDevice->CreateVertexShader(pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), nullptr, &m_pVertexShader);

        // 入力レイアウト
        D3D11_INPUT_ELEMENT_DESC layout[] = {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        };

        m_pDevice->CreateInputLayout(layout, ARRAYSIZE(layout),
            pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), &m_pInputLayout);
        pVSBlob->Release();

        // ピクセルシェーダー
        ID3DBlob* pPSBlob = nullptr;
        hr = D3DCompileFromFile(L"shader.hlsl", nullptr, nullptr, "PixelShaderPolygon", "ps_4_0_level_9_3",
            D3DCOMPILE_ENABLE_STRICTNESS, 0, &pPSBlob, &pErrorBlob);
        if (FAILED(hr)) {
            if (pErrorBlob) {
                MessageBoxA(nullptr, (char*)pErrorBlob->GetBufferPointer(), "PS Error", MB_OK);
                pErrorBlob->Release();
            }
            return false;
        }

        m_pDevice->CreatePixelShader(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), nullptr, &m_pPixelShader);
        pPSBlob->Release();

        // 定数バッファ
        D3D11_BUFFER_DESC cbDesc = {};
        cbDesc.ByteWidth = sizeof(XMMATRIX);
        cbDesc.Usage = D3D11_USAGE_DEFAULT;
        cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        m_pDevice->CreateBuffer(&cbDesc, nullptr, &m_pConstantBuffer);
        m_pContext->VSSetConstantBuffers(0, 1, &m_pConstantBuffer);

        // マテリアルバッファ（16byte境界に注意）
        cbDesc.ByteWidth = 80; // MaterialData構造体のサイズ
        m_pDevice->CreateBuffer(&cbDesc, nullptr, &m_pMaterialBuffer);
        m_pContext->VSSetConstantBuffers(1, 1, &m_pMaterialBuffer);

        // シェーダー設定
        m_pContext->IASetInputLayout(m_pInputLayout);
        m_pContext->VSSetShader(m_pVertexShader, nullptr, 0);
        m_pContext->PSSetShader(m_pPixelShader, nullptr, 0);

        return true;
    }

    void Renderer::Finalize() {
        if (m_pConstantBuffer) { m_pConstantBuffer->Release(); m_pConstantBuffer = nullptr; }
        if (m_pMaterialBuffer) { m_pMaterialBuffer->Release(); m_pMaterialBuffer = nullptr; }
        if (m_pInputLayout) { m_pInputLayout->Release(); m_pInputLayout = nullptr; }
        if (m_pVertexShader) { m_pVertexShader->Release(); m_pVertexShader = nullptr; }
        if (m_pPixelShader) { m_pPixelShader->Release(); m_pPixelShader = nullptr; }
        if (m_pDepthStateEnable) { m_pDepthStateEnable->Release(); m_pDepthStateEnable = nullptr; }
        if (m_pDepthStateDisable) { m_pDepthStateDisable->Release(); m_pDepthStateDisable = nullptr; }
        if (m_pDepthStencilView) { m_pDepthStencilView->Release(); m_pDepthStencilView = nullptr; }
        if (m_pRenderTargetView) { m_pRenderTargetView->Release(); m_pRenderTargetView = nullptr; }
        if (m_pSwapChain) { m_pSwapChain->Release(); m_pSwapChain = nullptr; }
        if (m_pContext) { m_pContext->ClearState(); m_pContext->Release(); m_pContext = nullptr; }
        if (m_pDevice) { m_pDevice->Release(); m_pDevice = nullptr; }
    }

    void Renderer::Clear(float r, float g, float b, float a) {
        float clearColor[4] = { r, g, b, a };
        m_pContext->ClearRenderTargetView(m_pRenderTargetView, clearColor);
        m_pContext->ClearDepthStencilView(m_pDepthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);
    }

    void Renderer::Present() {
        m_pSwapChain->Present(0, 0);
    }

    void Renderer::SetDepthEnable(bool enable) {
        m_pContext->OMSetDepthStencilState(
            enable ? m_pDepthStateEnable : m_pDepthStateDisable, 0);
    }

    void Renderer::SetWorldMatrix(const XMMATRIX& matrix) {
        m_worldMatrix = matrix;
        UpdateConstantBuffer();
    }

    void Renderer::SetViewMatrix(const XMMATRIX& matrix) {
        m_viewMatrix = matrix;
        UpdateConstantBuffer();
    }

    void Renderer::SetProjectionMatrix(const XMMATRIX& matrix) {
        m_projectionMatrix = matrix;
        UpdateConstantBuffer();
    }

    void Renderer::UpdateConstantBuffer() {
        XMMATRIX wvp = m_worldMatrix * m_viewMatrix * m_projectionMatrix;
        wvp = XMMatrixTranspose(wvp);

        XMFLOAT4X4 matrix;
        XMStoreFloat4x4(&matrix, wvp);
        m_pContext->UpdateSubresource(m_pConstantBuffer, 0, nullptr, &matrix, 0, 0);
    }

    void Renderer::SetupFor2D() {
        m_projectionMatrix = XMMatrixOrthographicOffCenterLH(
            0.0f, static_cast<float>(m_screenWidth),
            static_cast<float>(m_screenHeight), 0.0f,
            0.0f, 1.0f
        );
        m_viewMatrix = XMMatrixIdentity();
        m_worldMatrix = XMMatrixIdentity();
        SetDepthEnable(false);
    }

    void Renderer::SetupFor3D() {
        SetDepthEnable(true);
    }
}
