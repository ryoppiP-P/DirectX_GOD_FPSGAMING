// system/graphics/primitive.h
#pragma once

#include "../core/renderer.h"
#include "vertex.h"

namespace Engine {
    // Box vertex data (36 vertices)
    extern Vertex3D BoxVertices[36];

    // Primitive init/uninit
    void InitPrimitives(ID3D11Device* pDevice);
    void UninitPrimitives();

    // Default texture
    ID3D11ShaderResourceView* GetDefaultTexture();
}
