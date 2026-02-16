/*********************************************************************
  \file    ゲームマネージャー [game_manager.h]
  
  \Author  Ryoto Kikuchi
  \data    2025
 *********************************************************************/
#pragma once

#include "main.h"
#include <vector>

// 前方宣言
class GameObject;
class Map;
class MapRenderer;
class Player;

class GameManager {
public:
    // シングルトンアクセス
    static GameManager& Instance();

    // ライフサイクル管理
    HRESULT Initialize(HINSTANCE hInstance, HWND hWnd, BOOL bWindow);
    void Finalize();
    void Update();
    void Draw();

    // ワールドオブジェクト管理
    std::vector<GameObject*>& GetWorldObjects() { return m_worldObjects; }
    const std::vector<GameObject*>& GetWorldObjects() const { return m_worldObjects; }

    // 主要オブジェクト参照
    Map* GetMap() const { return m_pMap; }
    MapRenderer* GetMapRenderer() const { return m_pMapRenderer; }
    GameObject* GetLocalPlayerGameObject() const;

    // FPS情報
    double GetFPS() const { return m_fps; }

private:
    GameManager();
    ~GameManager();
    GameManager(const GameManager&) = delete;
    GameManager& operator=(const GameManager&) = delete;

    // ゲームオブジェクト
    Map* m_pMap = nullptr;
    MapRenderer* m_pMapRenderer = nullptr;
    std::vector<GameObject*> m_worldObjects;

    // 内部状態
    double m_fps = 0.0;
    uint32_t m_inputSeq = 0;
};
