#pragma once

#include "vertex.h"
#include "material.h"
#include <d3d11.h>
#include <DirectXMath.h>

namespace Engine {
    using namespace DirectX;

    class Sprite3D {
    public:
        static bool Initialize(ID3D11Device* pDevice);
        static void Finalize();

        // 3D空間に配置するスプライト
        static void Draw(
            ID3D11DeviceContext* pContext,
            ID3D11Buffer* pMaterialBuffer,
            const XMFLOAT3& position,
            const XMFLOAT2& size,
            const XMFLOAT4& color,
            ID3D11ShaderResourceView* pTexture = nullptr,
            const XMFLOAT3& rotation = { 0.0f, 0.0f, 0.0f }
        );

        // ビルボード（常にカメラを向く）
        static void DrawBillboard(
            ID3D11DeviceContext* pContext,
            ID3D11Buffer* pMaterialBuffer,
            const XMFLOAT3& position,
            const XMFLOAT2& size,
            const XMFLOAT4& color,
            const XMMATRIX& viewMatrix,
            ID3D11ShaderResourceView* pTexture = nullptr
        );

    private:
        static ID3D11Buffer* s_pVertexBuffer;
    };
}
