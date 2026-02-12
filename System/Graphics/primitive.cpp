// system/graphics/primitive.cpp
#include "primitive.h"
#include "texture_loader.h"
#include "../../polygon.h"
#include <cstring>
#include <cstddef>

namespace Engine {
    static ID3D11ShaderResourceView* g_pDefaultTexture = nullptr;

    Vertex3D BoxVertices[36] = {
        // Front (Z = -0.5)
        {{-0.5f,  0.5f, -0.5f}, {0, 0, -1}, {1, 1, 1, 1}, {0, 0}},
        {{ 0.5f,  0.5f, -0.5f}, {0, 0, -1}, {1, 1, 1, 1}, {1, 0}},
        {{-0.5f, -0.5f, -0.5f}, {0, 0, -1}, {1, 1, 1, 1}, {0, 1}},
        {{-0.5f, -0.5f, -0.5f}, {0, 0, -1}, {1, 1, 1, 1}, {0, 1}},
        {{ 0.5f,  0.5f, -0.5f}, {0, 0, -1}, {1, 1, 1, 1}, {1, 0}},
        {{ 0.5f, -0.5f, -0.5f}, {0, 0, -1}, {1, 1, 1, 1}, {1, 1}},
        // Back (Z = 0.5)
        {{ 0.5f,  0.5f,  0.5f}, {0, 0, 1}, {1, 1, 1, 1}, {0, 0}},
        {{-0.5f,  0.5f,  0.5f}, {0, 0, 1}, {1, 1, 1, 1}, {1, 0}},
        {{ 0.5f, -0.5f,  0.5f}, {0, 0, 1}, {1, 1, 1, 1}, {0, 1}},
        {{ 0.5f, -0.5f,  0.5f}, {0, 0, 1}, {1, 1, 1, 1}, {0, 1}},
        {{-0.5f,  0.5f,  0.5f}, {0, 0, 1}, {1, 1, 1, 1}, {1, 0}},
        {{-0.5f, -0.5f,  0.5f}, {0, 0, 1}, {1, 1, 1, 1}, {1, 1}},
        // Left (X = -0.5)
        {{-0.5f,  0.5f,  0.5f}, {-1, 0, 0}, {1, 1, 1, 1}, {0, 0}},
        {{-0.5f,  0.5f, -0.5f}, {-1, 0, 0}, {1, 1, 1, 1}, {1, 0}},
        {{-0.5f, -0.5f,  0.5f}, {-1, 0, 0}, {1, 1, 1, 1}, {0, 1}},
        {{-0.5f, -0.5f,  0.5f}, {-1, 0, 0}, {1, 1, 1, 1}, {0, 1}},
        {{-0.5f,  0.5f, -0.5f}, {-1, 0, 0}, {1, 1, 1, 1}, {1, 0}},
        {{-0.5f, -0.5f, -0.5f}, {-1, 0, 0}, {1, 1, 1, 1}, {1, 1}},
        // Right (X = 0.5)
        {{ 0.5f,  0.5f, -0.5f}, {1, 0, 0}, {1, 1, 1, 1}, {0, 0}},
        {{ 0.5f,  0.5f,  0.5f}, {1, 0, 0}, {1, 1, 1, 1}, {1, 0}},
        {{ 0.5f, -0.5f, -0.5f}, {1, 0, 0}, {1, 1, 1, 1}, {0, 1}},
        {{ 0.5f, -0.5f, -0.5f}, {1, 0, 0}, {1, 1, 1, 1}, {0, 1}},
        {{ 0.5f,  0.5f,  0.5f}, {1, 0, 0}, {1, 1, 1, 1}, {1, 0}},
        {{ 0.5f, -0.5f,  0.5f}, {1, 0, 0}, {1, 1, 1, 1}, {1, 1}},
        // Top (Y = 0.5)
        {{-0.5f,  0.5f,  0.5f}, {0, 1, 0}, {1, 1, 1, 1}, {0, 0}},
        {{ 0.5f,  0.5f,  0.5f}, {0, 1, 0}, {1, 1, 1, 1}, {1, 0}},
        {{-0.5f,  0.5f, -0.5f}, {0, 1, 0}, {1, 1, 1, 1}, {0, 1}},
        {{-0.5f,  0.5f, -0.5f}, {0, 1, 0}, {1, 1, 1, 1}, {0, 1}},
        {{ 0.5f,  0.5f,  0.5f}, {0, 1, 0}, {1, 1, 1, 1}, {1, 0}},
        {{ 0.5f,  0.5f, -0.5f}, {0, 1, 0}, {1, 1, 1, 1}, {1, 1}},
        // Bottom (Y = -0.5)
        {{-0.5f, -0.5f, -0.5f}, {0, -1, 0}, {1, 1, 1, 1}, {0, 0}},
        {{ 0.5f, -0.5f, -0.5f}, {0, -1, 0}, {1, 1, 1, 1}, {1, 0}},
        {{-0.5f, -0.5f,  0.5f}, {0, -1, 0}, {1, 1, 1, 1}, {0, 1}},
        {{-0.5f, -0.5f,  0.5f}, {0, -1, 0}, {1, 1, 1, 1}, {0, 1}},
        {{ 0.5f, -0.5f, -0.5f}, {0, -1, 0}, {1, 1, 1, 1}, {1, 0}},
        {{ 0.5f, -0.5f,  0.5f}, {0, -1, 0}, {1, 1, 1, 1}, {1, 1}},
    };

    void InitPrimitives(ID3D11Device* pDevice) {
        g_pDefaultTexture = TextureLoader::Load(pDevice, L"resource/texture/white.png");
        if (!g_pDefaultTexture) {
            g_pDefaultTexture = TextureLoader::CreateWhite(pDevice);
        }
    }

    void UninitPrimitives() {
        if (g_pDefaultTexture) {
            g_pDefaultTexture->Release();
            g_pDefaultTexture = nullptr;
        }
    }

    ID3D11ShaderResourceView* GetDefaultTexture() {
        return g_pDefaultTexture;
    }
}

// Global Box vertex data (VERTEX_3D type from polygon.h)
VERTEX_3D Box[36];

// Static initializer to copy from Engine::BoxVertices
namespace {
    struct BoxInitializer {
        BoxInitializer() {
            static_assert(sizeof(VERTEX_3D) == sizeof(Engine::Vertex3D), "VERTEX_3D and Engine::Vertex3D must have same size");
            static_assert(offsetof(VERTEX_3D, position) == offsetof(Engine::Vertex3D, position), "position offset mismatch");
            static_assert(offsetof(VERTEX_3D, normal) == offsetof(Engine::Vertex3D, normal), "normal offset mismatch");
            static_assert(offsetof(VERTEX_3D, color) == offsetof(Engine::Vertex3D, color), "color offset mismatch");
            static_assert(offsetof(VERTEX_3D, texCoord) == offsetof(Engine::Vertex3D, texCoord), "texCoord offset mismatch");
            std::memcpy(Box, Engine::BoxVertices, sizeof(Box));
        }
    } g_boxInit;
}

// Global function implementations for polygon.h
void InitPolygon() { Engine::InitPrimitives(Engine::GetDevice()); }
void UninitPolygon() { Engine::UninitPrimitives(); }
ID3D11ShaderResourceView* GetPolygonTexture() { return Engine::GetDefaultTexture(); }
