// system/graphics/primitive.h
#pragma once

#include "../core/renderer.h"
#include "vertex.h"

namespace Engine {
    // �����̂̒��_�f�[�^�i36���_�j
    extern Vertex3D BoxVertices[36];

    // �v���~�e�B�u�������E�I��
    void InitPrimitives(ID3D11Device* pDevice);
    void UninitPrimitives();

    // �f�t�H���g�e�N�X�`���擾
    ID3D11ShaderResourceView* GetDefaultTexture();
}

// �O���[�o�����J
extern Engine::Vertex3D Box[36];
inline void InitPolygon() { Engine::InitPrimitives(Engine::GetDevice()); }
inline void UninitPolygon() { Engine::UninitPrimitives(); }
inline ID3D11ShaderResourceView* GetPolygonTexture() { return Engine::GetDefaultTexture(); }
