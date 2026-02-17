/*********************************************************************
  \file    ゲームマネージャー [game_manager.h]
  
  \Author  Ryoto Kikuchi
  \data    2025
 *********************************************************************/
#pragma once

#include "main.h"
#include "Game/game.h"
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

    // シーン遷移管理へのアクセス
    Game& GetGame() { return m_game; }
    const Game& GetGame() const { return m_game; }

    // ワールドオブジェクト管理（現在のゲームシーン経由）
    std::vector<GameObject*>& GetWorldObjects();
    const std::vector<GameObject*>& GetWorldObjects() const;

    // 主要オブジェクト参照（現在のゲームシーン経由）
    Map* GetMap() const;
    MapRenderer* GetMapRenderer() const;
    GameObject* GetLocalPlayerGameObject() const;

    // FPS情報
    double GetFPS() const { return m_fps; }

private:
    GameManager();
    ~GameManager();
    GameManager(const GameManager&) = delete;
    GameManager& operator=(const GameManager&) = delete;

    // シーン遷移管理
    Game m_game;

    // 内部状態
    double m_fps = 0.0;
    uint32_t m_inputSeq = 0;

    // ワールドオブジェクト（ゲームシーンが無い場合のフォールバック用）
    static std::vector<GameObject*> s_emptyWorldObjects;
};
