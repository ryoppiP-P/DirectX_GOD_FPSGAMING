#pragma once

#include "vertex.h"
#include <d3d11.h>
#include <DirectXMath.h>

namespace Engine {
    using namespace DirectX;

    class Sprite2D {
    public:
        static bool Initialize(ID3D11Device* pDevice);
        static void Finalize();

        static void Draw(
            ID3D11DeviceContext* pContext,
            ID3D11Buffer* pMaterialBuffer,
            const XMFLOAT2& position,
            const XMFLOAT2& size,
            const XMFLOAT4& color,
            ID3D11ShaderResourceView* pTexture = nullptr,
            float rotation = 0.0f
        );

    private:
        static ID3D11Buffer* s_pVertexBuffer;
    };
}
