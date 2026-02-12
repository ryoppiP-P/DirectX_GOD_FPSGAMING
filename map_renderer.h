/*********************************************************************
  \file    マップ描画システム [map_renderer.h]
  
  \Author  Ryoto Kikuchi
  \data    2025/9/29
 *********************************************************************/
#pragma once

#include "main.h"
#include "renderer.h"
#include "map.h"

//*****************************************************************************
// マクロ定義
//*****************************************************************************
#define BOX_SIZE 1.0f  // 1つのボックスのサイズ

//*****************************************************************************
// クラス定義
//*****************************************************************************
class MapRenderer
{
private:
    // 立方体の頂点バッファ
    ID3D11Buffer* m_VertexBuffer;

    // インデックスバッファ（Unity方式対応）
    ID3D11Buffer* m_IndexBuffer;

    // テクスチャリソース
    ID3D11ShaderResourceView* m_Texture;

    // マップデータへの参照
    Map* m_pMap;

    // 立方体の頂点・インデックスバッファを作成（Unity方式: 24頂点+36インデックス）
    void CreateCubeBuffers();

public:
    // コンストラクタ・デストラクタ
    MapRenderer();
    ~MapRenderer();

    // 初期化・終了処理
    HRESULT Initialize(Map* pMap);
    void Uninitialize();

    // 描画処理
    void Draw();

    // 個別のボックス描画
    void DrawBox(float x, float y, float z);

    // ワールド座標の計算
    XMFLOAT3 GetWorldPosition(int mapX, int mapY, int mapZ) const;
};