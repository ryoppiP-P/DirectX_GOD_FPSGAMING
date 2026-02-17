#include "pch.h"
#include "material.h"

namespace Engine {
    Material::Material() {
    }

    Material::~Material() {
        if (m_ownsTexture && m_pTexture) {
            m_pTexture->Release();
            m_pTexture = nullptr;
        }
    }

    Material::Material(Material&& other) noexcept
        : m_data(other.m_data)
        , m_pTexture(other.m_pTexture)
        , m_ownsTexture(other.m_ownsTexture) {
        other.m_pTexture = nullptr;
        other.m_ownsTexture = false;
    }

    Material& Material::operator=(Material&& other) noexcept {
        if (this != &other) {
            if (m_ownsTexture && m_pTexture) {
                m_pTexture->Release();
            }

            m_data = other.m_data;
            m_pTexture = other.m_pTexture;
            m_ownsTexture = other.m_ownsTexture;

            other.m_pTexture = nullptr;
            other.m_ownsTexture = false;
        }
        return *this;
    }

    void Material::SetDiffuse(const XMFLOAT4& color) {
        m_data.diffuse = color;
    }

    void Material::SetAmbient(const XMFLOAT4& color) {
        m_data.ambient = color;
    }

    void Material::SetSpecular(const XMFLOAT4& color, float shininess) {
        m_data.specular = color;
        m_data.shininess = shininess;
    }

    void Material::SetEmission(const XMFLOAT4& color) {
        m_data.emission = color;
    }

    void Material::SetTexture(ID3D11ShaderResourceView* pTexture) {
        if (m_ownsTexture && m_pTexture) {
            m_pTexture->Release();
        }
        m_pTexture = pTexture;
        m_ownsTexture = false;  // ŠO•”‚©‚çŽó‚¯Žæ‚Á‚½‚Ì‚ÅŠ—L‚µ‚È‚¢
    }

    void Material::Apply(ID3D11DeviceContext* pContext, ID3D11Buffer* pMaterialBuffer) {
        if (!pContext) return;

        if (pMaterialBuffer) {
            pContext->UpdateSubresource(pMaterialBuffer, 0, nullptr, &m_data, 0, 0);
        }

        if (m_pTexture) {
            pContext->PSSetShaderResources(0, 1, &m_pTexture);
        }
    }

    std::shared_ptr<Material> Material::CreateDefault() {
        auto pMaterial = std::make_shared<Material>();
        pMaterial->SetDiffuse({ 1.0f, 1.0f, 1.0f, 1.0f });
        return pMaterial;
    }
}
