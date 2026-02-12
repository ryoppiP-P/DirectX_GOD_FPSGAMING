#include "texture_loader.h"
#include "../../external/DirectXTex.h"

namespace Engine {
    ID3D11ShaderResourceView* TextureLoader::Load(
        ID3D11Device* pDevice,
        const std::wstring& filePath) {
        if (!pDevice) return nullptr;

        DirectX::TexMetadata metadata;
        DirectX::ScratchImage image;

        HRESULT hr = DirectX::LoadFromWICFile(
            filePath.c_str(),
            DirectX::WIC_FLAGS_NONE,
            &metadata,
            image
        );

        if (FAILED(hr)) {
            return CreateWhite(pDevice);
        }

        ID3D11ShaderResourceView* pTexture = nullptr;
        hr = DirectX::CreateShaderResourceView(
            pDevice,
            image.GetImages(),
            image.GetImageCount(),
            metadata,
            &pTexture
        );

        if (FAILED(hr)) {
            return CreateWhite(pDevice);
        }

        return pTexture;
    }

    ID3D11ShaderResourceView* TextureLoader::CreateWhite(ID3D11Device* pDevice) {
        if (!pDevice) return nullptr;

        D3D11_TEXTURE2D_DESC desc = {};
        desc.Width = 1;
        desc.Height = 1;
        desc.MipLevels = 1;
        desc.ArraySize = 1;
        desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        desc.SampleDesc.Count = 1;
        desc.Usage = D3D11_USAGE_DEFAULT;
        desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

        uint32_t white = 0xFFFFFFFF;
        D3D11_SUBRESOURCE_DATA initData = {};
        initData.pSysMem = &white;
        initData.SysMemPitch = sizeof(uint32_t);

        ID3D11Texture2D* pTex = nullptr;
        HRESULT hr = pDevice->CreateTexture2D(&desc, &initData, &pTex);
        if (FAILED(hr)) return nullptr;

        ID3D11ShaderResourceView* pSRV = nullptr;
        hr = pDevice->CreateShaderResourceView(pTex, nullptr, &pSRV);
        pTex->Release();

        if (FAILED(hr)) return nullptr;

        return pSRV;
    }
}
