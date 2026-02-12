/*********************************************************************
  \file    renderer_globals.cpp

  renderer.h で宣言されたグローバル関数の実装。
  内部で Engine::Renderer を使用して処理を行う。
 *********************************************************************/
#include "renderer.h"
#include "System/Core/renderer.h"
#include "System/Graphics/material.h"

// デバイスアクセス
ID3D11Device* GetDevice() {
    return Engine::Renderer::GetInstance().GetDevice();
}

ID3D11DeviceContext* GetDeviceContext() {
    return Engine::Renderer::GetInstance().GetContext();
}

// レンダラー初期化/終了
HRESULT InitRenderer(HINSTANCE hInstance, HWND hWnd, BOOL bWindow) {
    return Engine::Renderer::GetInstance().Initialize(hInstance, hWnd, bWindow != FALSE) ? S_OK : E_FAIL;
}

void UninitRenderer() {
    Engine::Renderer::GetInstance().Finalize();
}

// フレーム処理
void Clear() {
    Engine::Renderer::GetInstance().Clear();
}

void Present() {
    Engine::Renderer::GetInstance().Present();
}

// 行列設定
void SetWorldMatrix(const DirectX::XMMATRIX& matrix) {
    Engine::Renderer::GetInstance().SetWorldMatrix(matrix);
}

void SetViewMatrix(const DirectX::XMMATRIX& matrix) {
    Engine::Renderer::GetInstance().SetViewMatrix(matrix);
}

void SetProjectionMatrix(const DirectX::XMMATRIX& matrix) {
    Engine::Renderer::GetInstance().SetProjectionMatrix(matrix);
}

// 深度設定
void SetDepthEnable(bool enable) {
    Engine::Renderer::GetInstance().SetDepthEnable(enable);
}

// マテリアル設定
void SetMaterial(const MATERIAL& mat) {
    ID3D11DeviceContext* ctx = Engine::Renderer::GetInstance().GetContext();
    ID3D11Buffer* buf = Engine::Renderer::GetInstance().GetMaterialBuffer();
    if (ctx && buf) {
        Engine::MaterialData data = {};
        data.diffuse = mat.Diffuse;
        data.ambient = mat.Ambient;
        data.specular = mat.Specular;
        data.emission = mat.Emission;
        data.shininess = mat.Shininess;
        ctx->UpdateSubresource(buf, 0, nullptr, &data, 0, 0);
    }
}
