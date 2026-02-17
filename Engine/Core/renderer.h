#pragma once

#include <d3d11.h>
#include <DirectXMath.h>
#include <wrl/client.h>

namespace Engine {
    using namespace DirectX;
    using Microsoft::WRL::ComPtr;

    // シングルトン描画システム
    class Renderer {
    public:
        static Renderer& GetInstance();

        // 初期化/終了
        bool Initialize(HINSTANCE hInstance, HWND hWnd, bool windowed);
        void Finalize();

        // フレーム操作
        void Clear(float r = 0.4f, float g = 0.2f, float b = 0.2f, float a = 1.0f);
        void Present();

        // 深度設定
        void SetDepthEnable(bool enable);

        // 行列設定
        void SetWorldMatrix(const XMMATRIX& matrix);
        void SetViewMatrix(const XMMATRIX& matrix);
        void SetProjectionMatrix(const XMMATRIX& matrix);

        // 2D/3D切り替え
        void SetupFor2D();
        void SetupFor3D();

        // アクセサ
        ID3D11Device* GetDevice() const { return m_pDevice.Get(); }
        ID3D11DeviceContext* GetContext() const { return m_pContext.Get(); }
        ID3D11Buffer* GetMaterialBuffer() const { return m_pMaterialBuffer.Get(); }

        uint32_t GetScreenWidth() const { return m_screenWidth; }
        uint32_t GetScreenHeight() const { return m_screenHeight; }

        IDXGISwapChain* GetSwapChain() const { return m_pSwapChain.Get(); }

    private:
        Renderer() = default;
        ~Renderer() = default;
        Renderer(const Renderer&) = delete;
        Renderer& operator=(const Renderer&) = delete;

        void UpdateConstantBuffer();

        // D3D11オブジェクト (ComPtr管理)
        ComPtr<ID3D11Device> m_pDevice;
        ComPtr<ID3D11DeviceContext> m_pContext;
        ComPtr<IDXGISwapChain> m_pSwapChain;
        ComPtr<ID3D11RenderTargetView> m_pRenderTargetView;
        ComPtr<ID3D11DepthStencilView> m_pDepthStencilView;

        ComPtr<ID3D11VertexShader> m_pVertexShader;
        ComPtr<ID3D11PixelShader> m_pPixelShader;
        ComPtr<ID3D11InputLayout> m_pInputLayout;
        ComPtr<ID3D11Buffer> m_pConstantBuffer;
        ComPtr<ID3D11Buffer> m_pMaterialBuffer;

        ComPtr<ID3D11DepthStencilState> m_pDepthStateEnable;
        ComPtr<ID3D11DepthStencilState> m_pDepthStateDisable;

        // 行列
        XMMATRIX m_worldMatrix = XMMatrixIdentity();
        XMMATRIX m_viewMatrix = XMMatrixIdentity();
        XMMATRIX m_projectionMatrix = XMMatrixIdentity();

        // 画面サイズ
        uint32_t m_screenWidth = 1920;
        uint32_t m_screenHeight = 1080;
    };

    // 便利なグローバルアクセス関数
    inline ID3D11Device* GetDevice() { return Renderer::GetInstance().GetDevice(); }
    inline ID3D11DeviceContext* GetDeviceContext() { return Renderer::GetInstance().GetContext(); }
}
