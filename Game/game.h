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

        virtual HRESULT Initialize() = 0;
        virtual void    Finalize() = 0;
        virtual void    Update() = 0;
        virtual void    Draw() = 0;

        SceneType GetSceneType() const { return m_sceneType; }

    protected:
        SceneType m_sceneType;
    };

    //*****************************************************************************
    // ゲームプレイシーン
    //*****************************************************************************
    class SceneGame : public Scene {
    public:
        SceneGame();
        ~SceneGame() override;

        HRESULT Initialize() override;
        void    Finalize()   override;
        void    Update()     override;
        void    Draw()       override;

        // ゲーム固有のアクセサ
        Map* GetMap()         const { return m_pMap; }
        MapRenderer* GetMapRenderer() const { return m_pMapRenderer; }
        std::vector<std::shared_ptr<GameObject>>& GetWorldObjects() { return m_worldObjects; }
        const std::vector<std::shared_ptr<GameObject>>& GetWorldObjects() const { return m_worldObjects; }

    private:
        Map* m_pMap = nullptr;
        MapRenderer* m_pMapRenderer = nullptr;
        std::vector<std::shared_ptr<GameObject>> m_worldObjects;

        // HP表示UIの描画（画面左上にHPバー＋テキスト）
        void DrawHPDisplay();
    };

    //*****************************************************************************
    // タイトルシーン（スタブ）
    //*****************************************************************************
    class SceneTitle : public Scene {
    public:
        SceneTitle();
        ~SceneTitle() override;

        HRESULT Initialize() override;
        void    Finalize()   override;
        void    Update()     override;
        void    Draw()       override;
    };

    //*****************************************************************************
    // リザルトシーン（スタブ）
    //*****************************************************************************
    class SceneResult : public Scene {
    public:
        SceneResult();
        ~SceneResult() override;

        HRESULT Initialize() override;
        void    Finalize()   override;
        void    Update()     override;
        void    Draw()       override;
    };

    //*****************************************************************************
    // シーン遷移管理
    //*****************************************************************************
    class Game {
    public:
        Game();
        ~Game();

        HRESULT Initialize(SceneType firstScene);
        void    Finalize();
        void    Update();
        void    Draw();

        void RequestSceneChange(SceneType nextScene);

        SceneType  GetCurrentSceneType() const { return m_currentSceneType; }
        Scene* GetCurrentScene()     const { return m_pCurrentScene.get(); }
        SceneGame* GetSceneGame()        const;

    private:
        void ExecuteSceneChange();
        std::unique_ptr<Scene> CreateScene(SceneType type);

        std::unique_ptr<Scene> m_pCurrentScene;
        SceneType m_currentSceneType = SceneType::NONE;
        SceneType m_nextSceneType = SceneType::NONE;
        bool      m_sceneChangeRequested = false;
    };

} // namespace Game
