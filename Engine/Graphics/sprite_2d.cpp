#include "pch.h"
#include "sprite_2d.h"
#include "material.h"
#include <cmath>

namespace Engine {
    ID3D11Buffer* Sprite2D::s_pVertexBuffer = nullptr;

    bool Sprite2D::Initialize(ID3D11Device* pDevice) {
        if (!pDevice) return false;

        D3D11_BUFFER_DESC bd = {};
        bd.Usage = D3D11_USAGE_DYNAMIC;
        bd.ByteWidth = sizeof(Vertex3D) * 4;
        bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

        HRESULT hr = pDevice->CreateBuffer(&bd, nullptr, &s_pVertexBuffer);
        return SUCCEEDED(hr);
    }

    void Sprite2D::Finalize() {
        if (s_pVertexBuffer) {
            s_pVertexBuffer->Release();
            s_pVertexBuffer = nullptr;
        }
    }

    void Sprite2D::Draw(
        ID3D11DeviceContext* pContext,
        ID3D11Buffer* pMaterialBuffer,
        const XMFLOAT2& position,
        const XMFLOAT2& size,
        const XMFLOAT4& color,
        ID3D11ShaderResourceView* pTexture,
        float rotation) {
        if (!pContext || !s_pVertexBuffer) return;

        float hw = size.x * 0.5f;
        float hh = size.y * 0.5f;

        // 回転行列
        float cosR = std::cos(rotation);
        float sinR = std::sin(rotation);

        auto RotatePoint = [&](float x, float y) -> XMFLOAT3 {
            return {
                position.x + x * cosR - y * sinR,
                position.y + x * sinR + y * cosR,
                0.0f
            };
            };

        D3D11_MAPPED_SUBRESOURCE msr;
        if (SUCCEEDED(pContext->Map(s_pVertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &msr))) {
            Vertex3D* vertices = static_cast<Vertex3D*>(msr.pData);

            vertices[0] = { RotatePoint(-hw, -hh), { 0, 0, 0 }, color, { 0, 0 } };
            vertices[1] = { RotatePoint(hw, -hh), { 0, 0, 0 }, color, { 1, 0 } };
            vertices[2] = { RotatePoint(-hw,  hh), { 0, 0, 0 }, color, { 0, 1 } };
            vertices[3] = { RotatePoint(hw,  hh), { 0, 0, 0 }, color, { 1, 1 } };

            pContext->Unmap(s_pVertexBuffer, 0);
        }

        // マテリアル設定
        if (pMaterialBuffer) {
            MaterialData mat = {};
            mat.diffuse = { 1, 1, 1, 1 };
            pContext->UpdateSubresource(pMaterialBuffer, 0, nullptr, &mat, 0, 0);
        }

        // テクスチャ設定
        if (pTexture) {
            pContext->PSSetShaderResources(0, 1, &pTexture);
        }

        // 描画
        UINT stride = sizeof(Vertex3D);
        UINT offset = 0;
        pContext->IASetVertexBuffers(0, 1, &s_pVertexBuffer, &stride, &offset);
        pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
        pContext->Draw(4, 0);
    }
}
