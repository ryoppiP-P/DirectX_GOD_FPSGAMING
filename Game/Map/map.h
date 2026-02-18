/*********************************************************************
  \file    3�����}�b�v�f�[�^�Ǘ� [map.h]
  
  \Author  Ryoto Kikuchi
  \data    2025/10/21
 *********************************************************************/
#pragma once

#include "main.h"
#include <vector>
#include <memory>
#include "Game/Objects/game_object.h"

namespace Game {

//*****************************************************************************
// �}�N����`
//*****************************************************************************
#define MAP_WIDTH  50    // �}�b�v�̕�
#define MAP_HEIGHT 50    // �}�b�v�̍���
#define MAP_DEPTH  50    // �}�b�v�̉��s��

//*****************************************************************************
// �\���̂̒�`
//*****************************************************************************
struct MapPosition
{
    int x, y, z;
};

//*****************************************************************************
// �N���X��`
//*****************************************************************************
class Map
{
private:
    // 3�����}�b�v�f�[�^�z�� [����][���s��][��]
    int mapData[MAP_HEIGHT][MAP_DEPTH][MAP_WIDTH];
    std::vector<std::unique_ptr<GameObject>> blockObjects; // �u���b�NGameObject���X�g

public:
    // �R���X�g���N�^�E�f�X�g���N�^
    Map();
    ~Map();

    // �������E�I������
    HRESULT Initialize(ID3D11ShaderResourceView* texture); // �e�N�X�`�����󂯎��悤�ύX
    void Uninitialize();

    // �}�b�v�f�[�^�A�N�Z�X
    int GetBlock(int x, int y, int z) const;
    void SetBlock(int x, int y, int z, int value);

    // �͈̓`�F�b�N
    bool IsValidPosition(int x, int y, int z) const;

    // �}�b�v�T�C�Y�擾
    int GetWidth() const { return MAP_WIDTH; }
    int GetHeight() const { return MAP_HEIGHT; }
    int GetDepth() const { return MAP_DEPTH; }

    // �T���v���}�b�v�f�[�^�̐ݒ�
    void CreateSampleMap();

    // �u���b�NGameObject���X�g�擾
    const std::vector<std::unique_ptr<GameObject>>& GetBlockObjects() const { return blockObjects; }

    // *** �ǉ� ***
    float GetGroundHeight(float x, float z) const;
    // *** �ǉ� �I�� ***
   
    // �u���b�NGameObject����
    void GenerateBlockObjects(ID3D11ShaderResourceView* texture);
};

} // namespace Game