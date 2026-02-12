/*********************************************************************
  \file    3�����}�b�v�f�[�^�Ǘ� [map.cpp]
  
  \Author  Ryoto Kikuchi
  \data    2025/9/27
 *********************************************************************/
#include "map.h"
#include "game_object.h"
#include "map_renderer.h"
#include "System/Graphics/primitive.h"

 // �ǉ�: Clamp�֐�
template<typename T>
T Clamp(T value, T minValue, T maxValue)
{
    if (value < minValue) return minValue;
    if (value > maxValue) return maxValue;
    return value;
}
// �ǉ�

//=============================================================================
// �R���X�g���N�^
//=============================================================================
Map::Map() {
    // �}�b�v�f�[�^���������i�S��0�ŏ������j
    for (int y = 0; y < MAP_HEIGHT; y++) {
        for (int z = 0; z < MAP_DEPTH; z++) {
            for (int x = 0; x < MAP_WIDTH; x++) {
                mapData[y][z][x] = 0;
            }
        }
    }
}

//=============================================================================
// �f�X�g���N�^
//=============================================================================
Map::~Map() {
}

//=============================================================================
// ����������
//=============================================================================
HRESULT Map::Initialize(ID3D11ShaderResourceView* texture) {
    // �T���v���}�b�v�f�[�^���쐬
    CreateSampleMap();
    GenerateBlockObjects(texture);

    return S_OK;
}

//=============================================================================
// �I������
//=============================================================================
void Map::Uninitialize()
{
    // ���ɏ����Ȃ�
}

//=============================================================================
// �u���b�N�f�[�^�̎擾
//=============================================================================
int Map::GetBlock(int x, int y, int z) const
{
    if (!IsValidPosition(x, y, z)) {
        return 0;  // �͈͊O��0��Ԃ�
    }
    return mapData[y][z][x];
}

//=============================================================================
// �u���b�N�f�[�^�̐ݒ�
//=============================================================================
void Map::SetBlock(int x, int y, int z, int value)
{
    if (IsValidPosition(x, y, z)) {
        mapData[y][z][x] = value;
    }
}

//=============================================================================
// �ʒu�̗L�����`�F�b�N
//=============================================================================
bool Map::IsValidPosition(int x, int y, int z) const
{
    return (x >= 0 && x < MAP_WIDTH &&
            y >= 0 && y < MAP_HEIGHT &&
            z >= 0 && z < MAP_DEPTH);
}

//=============================================================================
// �T���v���}�b�v�f�[�^�̍쐬
//=============================================================================
void Map::CreateSampleMap()
{
    // 50x50x50�̃T���v���}�b�v�f�[�^
    // ��ʑw�iy=0�j- �O���̕�
    for (int z = 0; z < MAP_DEPTH; z++) {
        for (int x = 0; x < MAP_WIDTH; x++) {
			mapData[0][z][x] = 1;  // ��ʑS�̂�ǂ�
        }
    }
    // �����ɒ���ǉ�
    mapData[0][2][2] = 1;

    // ���ԑw�iy=1,2,3�j- �����I�ȕǂƍ\����
    for (int y = 1; y < MAP_HEIGHT - 1; y++) {
        // �l���̒�
        mapData[y][0][0] = 1;
        mapData[y][0][MAP_WIDTH - 1] = 1;
        mapData[y][MAP_DEPTH - 1][0] = 1;
        mapData[y][MAP_DEPTH - 1][MAP_WIDTH - 1] = 1;

        // �����̍\����
        if (y == 1 || y == 2) {
            mapData[y][2][2] = 1;  // ����������ɉ���
        }
        
        // ���x��2�ňꕔ�̕ǂ�ǉ�
        if (y == 2) {
            mapData[y][1][2] = 1;
            mapData[y][3][2] = 1;
            mapData[y][2][1] = 1;
            mapData[y][2][3] = 1;
        }
    }

    // �ŏ�w�iy=4�j- �����̂悤�ȍ\��
    for (int z = 0; z < MAP_DEPTH; z++) {
        for (int x = 0; x < MAP_WIDTH; x++) {
            if ((x == 0 || x == MAP_WIDTH - 1) && (z == 0 || z == MAP_DEPTH - 1)) {
                mapData[MAP_HEIGHT - 1][z][x] = 1;  // �l���̂�
            }
        }
    }
}

//void Map::CreateSampleMap()
//{
//    // 5x5x5 �̃T���v����z�񃊃e�����Œ�`�iy, z, x �̏��j
//    static const int sample[MAP_HEIGHT][MAP_DEPTH][MAP_WIDTH] = {
//        // y = 0
//        {
//            {1,1,1,1,1},
//            {1,0,0,0,1},
//            {1,0,1,0,1},
//            {1,0,0,0,1},
//            {1,1,1,1,1}
//        },
//        // y = 1
//        {
//            {1,0,0,0,1},
//            {0,0,0,0,0},
//            {0,0,1,0,0},
//            {0,0,0,0,0},
//            {1,0,0,0,1}
//        },
//        // y = 2
//        {
//            {1,0,0,0,1},
//            {0,0,1,0,0},
//            {0,1,1,1,0},
//            {0,0,1,0,0},
//            {1,0,0,0,1}
//        },
//        // y = 3
//        {
//            {1,0,0,0,1},
//            {0,0,0,0,0},
//            {0,0,0,0,0},
//            {0,0,0,0,0},
//            {1,0,0,0,1}
//        },
//        // y = 4
//        {
//            {1,0,0,0,1},
//            {0,0,0,0,0},
//            {0,0,0,0,0},
//            {0,0,0,0,0},
//            {1,0,0,0,1}
//        }
//    };
//
//    // �z���mapData�ɃR�s�[
//    for (int y = 0; y < MAP_HEIGHT; ++y) {
//        for (int z = 0; z < MAP_DEPTH; ++z) {
//            for (int x = 0; x < MAP_WIDTH; ++x) {
//                mapData[y][z][x] = sample[y][z][x];
//            }
//        }
//    }
//}

// *** �ǉ� ***
float Map::GetGroundHeight(float worldX, float worldZ) const
{
    float offsetX = -(MAP_WIDTH - 1) * BOX_SIZE * 0.5f;
    float offsetZ = -(MAP_DEPTH - 1) * BOX_SIZE * 0.5f;

    int mapX = static_cast<int>((worldX - offsetX) / BOX_SIZE);
    int mapZ = static_cast<int>((worldZ - offsetZ) / BOX_SIZE);

#if __cplusplus >= 201703L
    mapX = std::clamp(mapX, 0, MAP_WIDTH - 1);
    mapZ = std::clamp(mapZ, 0, MAP_DEPTH - 1);
#else
    mapX = Clamp(mapX, 0, MAP_WIDTH - 1);
    mapZ = Clamp(mapZ, 0, MAP_DEPTH - 1);
#endif

    for (int y = MAP_HEIGHT - 1; y >= 0; --y) {
        if (mapData[y][mapZ][mapX] == 1) {
            float offsetY = -(MAP_HEIGHT - 1) * BOX_SIZE * 0.5f;
            return y * BOX_SIZE + offsetY + BOX_SIZE * 0.5f;
        }
    }

    return 0.0f;
}
// *** �ǉ� �I�� ***


void Map::GenerateBlockObjects(ID3D11ShaderResourceView* texture)
{
    blockObjects.clear();
    float offsetX = -(MAP_WIDTH - 1) * BOX_SIZE * 0.5f;
    float offsetY = -(MAP_HEIGHT - 1) * BOX_SIZE * 0.5f;
    float offsetZ = -(MAP_DEPTH - 1) * BOX_SIZE * 0.5f;
    for (int y = 0; y < MAP_HEIGHT; y++) {
        for (int z = 0; z < MAP_DEPTH; z++) {
            for (int x = 0; x < MAP_WIDTH; x++) {
                if (mapData[y][z][x] == 1) {
                    auto obj = std::make_unique<GameObject>();
                    obj->position = XMFLOAT3(x * BOX_SIZE + offsetX, y * BOX_SIZE + offsetY, z * BOX_SIZE + offsetZ);
                    obj->scale = XMFLOAT3(BOX_SIZE, BOX_SIZE, BOX_SIZE);
                    obj->rotation = XMFLOAT3(0, 0, 0);
                    obj->setMesh(Box, 36, texture);
                    obj->setBoxCollider(obj->scale);
                    blockObjects.push_back(std::move(obj));
                }
            }
        }
    }
}