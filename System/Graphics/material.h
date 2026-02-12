#pragma once

#include <DirectXMath.h>
#include <d3d11.h>
#include <memory>
#include <string>

namespace Engine {
    using namespace DirectX;

    struct MaterialData {
        XMFLOAT4 ambient = { 0.2f, 0.2f, 0.2f, 1.0f };
        XMFLOAT4 diffuse = { 1.0f, 1.0f, 1.0f, 1.0f };
        XMFLOAT4 specular = { 0.0f, 0.0f, 0.0f, 1.0f };
        XMFLOAT4 emission = { 0.0f, 0.0f, 0.0f, 1.0f };
        float shininess = 0.0f;
        float padding[3] = { 0.0f, 0.0f, 0.0f };  // 16byte境界
    };

    class Material {
    public:
        Material();
        ~Material();

        Material(const Material&) = delete;
        Material& operator=(const Material&) = delete;
        Material(Material&& other) noexcept;
        Material& operator=(Material&& other) noexcept;

        // プロパティ設定
        void SetDiffuse(const XMFLOAT4& color);
        void SetAmbient(const XMFLOAT4& color);
        void SetSpecular(const XMFLOAT4& color, float shininess);
        void SetEmission(const XMFLOAT4& color);

        // テクスチャ設定
        void SetTexture(ID3D11ShaderResourceView* pTexture);
        ID3D11ShaderResourceView* GetTexture() const { return m_pTexture; }

        // GPU適用
        void Apply(ID3D11DeviceContext* pContext, ID3D11Buffer* pMaterialBuffer);

        // アクセサ
        const MaterialData& GetData() const { return m_data; }

        // ファクトリ
        static std::shared_ptr<Material> CreateDefault();

    private:
        MaterialData m_data;
        ID3D11ShaderResourceView* m_pTexture = nullptr;
        bool m_ownsTexture = false;
    };
}
