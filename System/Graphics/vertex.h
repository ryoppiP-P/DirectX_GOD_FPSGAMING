#pragma once

#include <DirectXMath.h>

namespace Engine {
    using namespace DirectX;

    // 基本3D頂点
    struct Vertex3D {
        XMFLOAT3 position;
        XMFLOAT3 normal;
        XMFLOAT4 color;
        XMFLOAT2 texCoord;

        Vertex3D()
            : position(0.0f, 0.0f, 0.0f)
            , normal(0.0f, 0.0f, 0.0f)
            , color(1.0f, 1.0f, 1.0f, 1.0f)
            , texCoord(0.0f, 0.0f) {
        }

        Vertex3D(const XMFLOAT3& pos, const XMFLOAT3& norm,
            const XMFLOAT4& col, const XMFLOAT2& uv)
            : position(pos)
            , normal(norm)
            , color(col)
            , texCoord(uv) {
        }
    };

    // 将来のスキンメッシュ用
    struct VertexSkinned {
        XMFLOAT3 position;
        XMFLOAT3 normal;
        XMFLOAT4 color;
        XMFLOAT2 texCoord;
        uint32_t boneIndices[4];
        float boneWeights[4];
    };

    // 2D頂点（スプライト用）
    struct Vertex2D {
        XMFLOAT3 position;
        XMFLOAT4 color;
        XMFLOAT2 texCoord;
    };
}
