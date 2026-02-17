#pragma once

#include "vertex.h"
#include <vector>
#include <memory>
#include <string>
#include <d3d11.h>

namespace Engine {
    class Mesh {
    public:
        Mesh();
        virtual ~Mesh();

        // コピー禁止、ムーブ許可
        Mesh(const Mesh&) = delete;
        Mesh& operator=(const Mesh&) = delete;
        Mesh(Mesh&& other) noexcept;
        Mesh& operator=(Mesh&& other) noexcept;

        // 頂点データ設定
        void SetVertices(const std::vector<Vertex3D>& vertices);
        void SetVertices(std::vector<Vertex3D>&& vertices);
        void SetIndices(const std::vector<uint32_t>& indices);
        void SetIndices(std::vector<uint32_t>&& indices);

        // GPUバッファ操作
        bool Upload(ID3D11Device* pDevice);
        void Bind(ID3D11DeviceContext* pContext);
        void Draw(ID3D11DeviceContext* pContext);

        // アクセサ
        const std::vector<Vertex3D>& GetVertices() const { return m_vertices; }
        const std::vector<uint32_t>& GetIndices() const { return m_indices; }
        uint32_t GetVertexCount() const { return static_cast<uint32_t>(m_vertices.size()); }
        uint32_t GetIndexCount() const { return static_cast<uint32_t>(m_indices.size()); }
        bool IsUploaded() const { return m_pVertexBuffer != nullptr; }
        bool HasIndices() const { return !m_indices.empty(); }

        // 境界情報（MeshCollider用）
        void GetBounds(XMFLOAT3& outMin, XMFLOAT3& outMax) const;

    protected:
        std::vector<Vertex3D> m_vertices;
        std::vector<uint32_t> m_indices;

        ID3D11Buffer* m_pVertexBuffer = nullptr;
        ID3D11Buffer* m_pIndexBuffer = nullptr;

        XMFLOAT3 m_boundsMin = { 0.0f, 0.0f, 0.0f };
        XMFLOAT3 m_boundsMax = { 0.0f, 0.0f, 0.0f };
        bool m_boundsDirty = true;

        void CalculateBounds();
        void ReleaseBuffers();
    };
}
