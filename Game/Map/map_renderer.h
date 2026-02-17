/*********************************************************************
  \file    �}�b�v�`��V�X�e�� [map_renderer.h]
  
  \Author  Ryoto Kikuchi
  \data    2025/9/29
 *********************************************************************/
#pragma once

#include "main.h"
#include "Engine/Core/renderer.h"
#include "Engine/Graphics/vertex.h"
#include "Engine/Graphics/material.h"
#include "map.h"

//*****************************************************************************
// �}�N����`
//*****************************************************************************
#define BOX_SIZE 1.0f  // 1�̃{�b�N�X�̃T�C�Y

//*****************************************************************************
// �N���X��`
//*****************************************************************************
class MapRenderer
{
private:
    // �����̂̒��_�o�b�t�@
    ID3D11Buffer* m_VertexBuffer;

    // �C���f�b�N�X�o�b�t�@�iUnity�����Ή��j
    ID3D11Buffer* m_IndexBuffer;

    // �e�N�X�`�����\�[�X
    ID3D11ShaderResourceView* m_Texture;

    // �}�b�v�f�[�^�ւ̎Q��
    Map* m_pMap;

    // �����̂̒��_�E�C���f�b�N�X�o�b�t�@���쐬�iUnity����: 24���_+36�C���f�b�N�X�j
    void CreateCubeBuffers();

public:
    // �R���X�g���N�^�E�f�X�g���N�^
    MapRenderer();
    ~MapRenderer();

    // �������E�I������
    HRESULT Initialize(Map* pMap);
    void Uninitialize();

    // �`�揈��
    void Draw();

    // �ʂ̃{�b�N�X�`��
    void DrawBox(float x, float y, float z);

    // ���[���h���W�̌v�Z
    XMFLOAT3 GetWorldPosition(int mapX, int mapY, int mapZ) const;
};