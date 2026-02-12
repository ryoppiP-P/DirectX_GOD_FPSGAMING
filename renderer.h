/*********************************************************************
  \file    レンダラー互換ヘッダー [renderer.h]
  
  System/Core/renderer.h をラップし、旧システムのグローバル関数を提供する
 *********************************************************************/
#pragma once

// 新システムのレンダラーをインクルード
#include "System/Core/renderer.h"
#include "System/Graphics/vertex.h"
#include "System/Graphics/material.h"

// 旧システム互換の型提供
using VERTEX_3D = Engine::Vertex3D;

// 旧システムの MATERIAL 構造体互換
struct MATERIAL {
    DirectX::XMFLOAT4 Diffuse;
    DirectX::XMFLOAT4 Ambient;
    DirectX::XMFLOAT4 Specular;
    DirectX::XMFLOAT4 Emission;
    float Shininess;
    float Padding[3];
};

// 旧システムのグローバル関数互換ラッパー
using namespace Engine;

// レンダラー初期化/終了
inline HRESULT InitRenderer(HINSTANCE hInstance, HWND hWnd, BOOL bWindow) {
    return Engine::Renderer::GetInstance().Initialize(hInstance, hWnd, bWindow != FALSE) ? S_OK : E_FAIL;
}

inline void UninitRenderer() {
    Engine::Renderer::GetInstance().Finalize();
}

// フレーム処理
inline void Clear() {
    Engine::Renderer::GetInstance().Clear();
}

inline void Present() {
    Engine::Renderer::GetInstance().Present();
}

// 行列設定
inline void SetWorldMatrix(const DirectX::XMMATRIX& matrix) {
    Engine::Renderer::GetInstance().SetWorldMatrix(matrix);
}

inline void SetViewMatrix(const DirectX::XMMATRIX& matrix) {
    Engine::Renderer::GetInstance().SetViewMatrix(matrix);
}

inline void SetProjectionMatrix(const DirectX::XMMATRIX& matrix) {
    Engine::Renderer::GetInstance().SetProjectionMatrix(matrix);
}

// 深度設定
inline void SetDepthEnable(bool enable) {
    Engine::Renderer::GetInstance().SetDepthEnable(enable);
}

// マテリアル設定（旧 MATERIAL 構造体対応）
inline void SetMaterial(const MATERIAL& mat) {
    auto* ctx = Engine::Renderer::GetInstance().GetContext();
    auto* buf = Engine::Renderer::GetInstance().GetMaterialBuffer();
    if (ctx && buf) {
        // MATERIAL を MaterialData へ変換
        Engine::MaterialData data = {};
        data.diffuse = mat.Diffuse;
        data.ambient = mat.Ambient;
        data.specular = mat.Specular;
        data.emission = mat.Emission;
        data.shininess = mat.Shininess;
        ctx->UpdateSubresource(buf, 0, nullptr, &data, 0, 0);
    }
}
