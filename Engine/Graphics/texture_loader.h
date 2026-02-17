#pragma once

#include <d3d11.h>
#include <string>

namespace Engine {
    class TextureLoader {
    public:
        static ID3D11ShaderResourceView* Load(
            ID3D11Device* pDevice,
            const std::wstring& filePath
        );

        static ID3D11ShaderResourceView* CreateWhite(ID3D11Device* pDevice);
    };
}
