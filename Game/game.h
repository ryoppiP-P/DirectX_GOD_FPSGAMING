/*********************************************************************
  \file    ゲームシーン管理 [game.h]

  \Author  Ryoto Kikuchi
  \data    2025
 *********************************************************************/
#pragma once

#include "main.h"
#include <vector>
#include <memory>

namespace Game {

// 前方宣言
class GameObject;
class Map;
class MapRenderer;

//*****************************************************************************
// シーン識別用列挙型
//*****************************************************************************
enum class SceneType {
    NONE,
    TITLE,
    GAME,
    RESULT,
    MAX
};

//*****************************************************************************
// シーン基底クラス
//*****************************************************************************
class Scene {
public:
    Scene() : m_sceneType(SceneType::NONE) {}
    virtual ~Scene() = default;

    // ライフサイクル（派生先でオーバーライド）
    virtual HRESULT Initialize() = 0;
    virtual void    Finalize() = 0;
    virtual void    Update() = 0;
    virtual void    Draw() = 0;

    SceneType GetSceneType() const { return m_sceneType; }

protected:
    SceneType m_sceneType;
};

//*****************************************************************************
// ゲームプレイシーン（既存のゲームロジックをラップ）
//*****************************************************************************
class SceneGame : public Scene {
public:
    SceneGame();
    ~SceneGame() override;

    HRESULT Initialize() override;
    void    Finalize() override;
    void    Update() override;
    void    Draw() override;

    // ゲーム固有のアクセサ
    Map* GetMap() const { return m_pMap; }
    MapRenderer* GetMapRenderer() const { return m_pMapRenderer; }
    std::vector<std::shared_ptr<GameObject>>& GetWorldObjects() { return m_worldObjects; }
    const std::vector<std::shared_ptr<GameObject>>& GetWorldObjects() const { return m_worldObjects; }

private:
    Map* m_pMap = nullptr;
    MapRenderer* m_pMapRenderer = nullptr;
    std::vector<std::shared_ptr<GameObject>> m_worldObjects;
};

//*****************************************************************************
// タイトルシーン（将来の拡張用スタブ）
//*****************************************************************************
class SceneTitle : public Scene {
public:
    SceneTitle();
    ~SceneTitle() override;

    HRESULT Initialize() override;
    void    Finalize() override;
    void    Update() override;
    void    Draw() override;
};

//*****************************************************************************
// リザルトシーン（将来の拡張用スタブ）
//*****************************************************************************
class SceneResult : public Scene {
public:
    SceneResult();
    ~SceneResult() override;

    HRESULT Initialize() override;
    void    Finalize() override;
    void    Update() override;
    void    Draw() override;
};

//*****************************************************************************
// シーン遷移を管理するGameクラス
//*****************************************************************************
class Game {
public:
    Game();
    ~Game();

    // 初回シーンの設定と初期化
    HRESULT Initialize(SceneType firstScene);
    void    Finalize();

    // 毎フレーム呼び出し
    void Update();
    void Draw();

    // シーン遷移要求（次フレームの先頭で切り替わる）
    void RequestSceneChange(SceneType nextScene);

    // 現在のシーン情報
    SceneType GetCurrentSceneType() const { return m_currentSceneType; }
    Scene*    GetCurrentScene() const { return m_pCurrentScene.get(); }

    // ゲームプレイシーンへの便利アクセサ（既存コードとの互換用）
    SceneGame* GetSceneGame() const;

private:
    // シーン切り替え実行
    void ExecuteSceneChange();

    // シーン生成ファクトリ
    std::unique_ptr<Scene> CreateScene(SceneType type);

    std::unique_ptr<Scene> m_pCurrentScene;
    SceneType m_currentSceneType = SceneType::NONE;
    SceneType m_nextSceneType = SceneType::NONE;
    bool m_sceneChangeRequested = false;
};

} // namespace Game
