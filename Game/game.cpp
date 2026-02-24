/*********************************************************************
  \file    ゲームシーン管理 [game.cpp]

  \Author  Ryoto Kikuchi
  \data    2025
 *********************************************************************/
#include "pch.h"
#include "game.h"
#include "Engine/Core/renderer.h"
#include "Engine/Graphics/vertex.h"
#include "Engine/Graphics/material.h"
#include "Engine/Graphics/primitive.h"
#include "Engine/Collision/collision_system.h"
#include "Engine/Collision/map_collision.h"
#include "Engine/Input/keyboard.h"
#include "Game/Map/map.h"
#include "Game/Map/map_renderer.h"
#include "Game/Objects/player.h"
#include "Game/Managers/player_manager.h"
#include "Game/Objects/camera.h"
#include "NetWork/network_manager.h"
#include "Game/Objects/bullet.h"
#include "Game/Objects/game_object.h"
#include "Engine/Core/timer.h"
#include <iostream>
#include <sstream>   // HP表示用の文字列組み立て

namespace Game {

    // 外部で寿命管理されるオブジェクトを非所有shared_ptrとしてラップする
    template<typename T>
    std::shared_ptr<T> MakeNonOwning(T* ptr) {
        return std::shared_ptr<T>(ptr, [](T*) {});
    }

    //===================================
    // マクロ定義
    //===================================
#define CLASS_NAME "DX21 Window"

//*****************************************************************************
// SceneGame 実装
//*****************************************************************************
    SceneGame::SceneGame() {
        m_sceneType = SceneType::GAME;
    }

    SceneGame::~SceneGame() {
    }

    HRESULT SceneGame::Initialize() {
        // === マップ初期化 ===
        m_pMap = new Map();
        if (m_pMap) {
            m_pMap->Initialize(Engine::GetDefaultTexture());
        }

        m_pMapRenderer = new MapRenderer();
        if (m_pMapRenderer) {
            m_pMapRenderer->Initialize(m_pMap);
        }

        // === プレイヤー選択ダイアログ ===
        HWND hWnd = FindWindowA(CLASS_NAME, nullptr);
        int msgRes = MessageBox(hWnd, "Choose starting player:\nYes = Player1 (TPS)\nNo = Player2 (FPS)",
            "Select Player", MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON1);
        if (msgRes == IDYES) {
            PlayerManager::GetInstance().SetInitialActivePlayer(1);
            std::cout << "[SceneGame] Selected initial player:1\n";
        } else {
            PlayerManager::GetInstance().SetInitialActivePlayer(2);
            std::cout << "[SceneGame] Selected initial player:2\n";
        }

        InitializePlayers(m_pMap, Engine::GetDefaultTexture());

        // === カメラ初期化 ===
        InitializeCameraSystem();

        // === ワールドオブジェクトにマップブロックを登録 ===
        m_worldObjects.clear();
        if (m_pMap) {
            const auto& blocks = m_pMap->GetBlockObjects();
            for (const auto& sp : blocks) {
                m_worldObjects.push_back(sp);
            }
        }

        // === 衝突システム初期化 ===
        Engine::CollisionSystem::GetInstance().Initialize();
        Engine::MapCollision::GetInstance().Initialize(2.0f);

        // マップブロックをMapCollisionに登録
        if (m_pMap) {
            const auto& blocks = m_pMap->GetBlockObjects();
            for (const auto& block : blocks) {
                Engine::MapCollision::GetInstance().RegisterBlock(block->GetBoxCollider());
            }
        }

        // =====================================================
        // 衝突コールバック: 弾がプレイヤーに当たった時の処理
        // - 自分が撃った弾は自分に当たらない (ownerPlayerId判定)
        // - 相手プレイヤーに1ダメージを与える
        // =====================================================
        Engine::CollisionSystem::GetInstance().SetCallback(
            [](const Engine::CollisionHit& hit) {
                Bullet* bullet = nullptr;
                Player* player = nullptr;

                // 衝突した2つのオブジェクトから弾とプレイヤーを特定
                if (Engine::HasFlag(hit.dataA->layer, Engine::CollisionLayer::PROJECTILE))
                    bullet = static_cast<Bullet*>(hit.dataA->userData);
                if (Engine::HasFlag(hit.dataB->layer, Engine::CollisionLayer::PROJECTILE))
                    bullet = static_cast<Bullet*>(hit.dataB->userData);
                if (Engine::HasFlag(hit.dataA->layer, Engine::CollisionLayer::PLAYER))
                    player = static_cast<Player*>(hit.dataA->userData);
                if (Engine::HasFlag(hit.dataB->layer, Engine::CollisionLayer::PLAYER))
                    player = static_cast<Player*>(hit.dataB->userData);

                // 弾とプレイヤーの両方が有効な場合のみ処理
                if (bullet && player && bullet->active && player->IsAlive()) {

                    // 自分が撃った弾は自分に当たらない
                    if (bullet->ownerPlayerId == player->GetPlayerId()) {
                        return;  // 自弾なのでスキップ
                    }

                    // 弾を消してプレイヤーに1ダメージ
                    bullet->Deactivate();
                    player->TakeDamage(1);

                    std::cout << "[Hit!] Player " << player->GetPlayerId()
                        << " took 1 damage, HP=" << player->GetHP()
                        << "/" << player->GetMaxHP() << "\n";

                    // HPが0になったら死亡ログ
                    if (!player->IsAlive()) {
                        std::cout << "[Kill!] Player " << player->GetPlayerId() << " has been eliminated!\n";
                    }
                }
            }
        );

        // ローカルプレイヤーのID初期設定
        GameObject* localGo = GetLocalPlayerGameObject();
        if (localGo) {
            localGo->setId(0);
            std::cout << "[SceneGame] Players initialized\n";
            std::cout << "[SceneGame] 1キー: Player1 (TPS視点), 2キー: Player2 (FPS視点)\n";
        }

        return S_OK;
    }

    void SceneGame::Finalize() {
        if (m_pMapRenderer) {
            m_pMapRenderer->Uninitialize();
            delete m_pMapRenderer;
            m_pMapRenderer = nullptr;
        }

        m_worldObjects.clear();

        if (m_pMap) {
            m_pMap->Uninitialize();
            delete m_pMap;
            m_pMap = nullptr;
        }

        Engine::CollisionSystem::GetInstance().Shutdown();
        Engine::MapCollision::GetInstance().Shutdown();
    }

    void SceneGame::Update() {
        static int frameCounter = 0;
        frameCounter++;

        // === F1: ホストとして起動 ===
        if (Keyboard_IsKeyDownTrigger(KK_F1)) {
            if (!g_network.is_host()) {
                if (g_network.start_as_host()) {
                    std::cout << "[SceneGame] Started as HOST - Waiting for clients...\n";
                } else {
                    std::cout << "[SceneGame] FAILED to start as HOST\n";
                }
            } else {
                std::cout << "[SceneGame] Already running as HOST\n";
            }
        }

        // === F2: クライアントとして起動 ===
        if (Keyboard_IsKeyDownTrigger(KK_F2)) {
            if (!g_network.is_host()) {
                if (g_network.start_as_client()) {
                    std::string hostIp;
                    if (g_network.discover_and_join(hostIp)) {
                        std::cout << "[SceneGame] Discovered host: " << hostIp << " - CLIENT CONNECTED!\n";
                    } else {
                        std::cout << "[SceneGame] CLIENT: Host discovery FAILED\n";
                    }
                } else {
                    std::cout << "[SceneGame] FAILED to start as CLIENT\n";
                }
            } else {
                std::cout << "[SceneGame] Cannot start client - already running as HOST\n";
            }
        }

        // === ネットワーク更新 ===
        GameObject* localGo = GetLocalPlayerGameObject();
        constexpr float fixedDt = 1.0f / 60.0f;
        g_network.update(fixedDt, localGo, m_worldObjects);

        // ネットワークオブジェクトの位置を毎フレーム補間で滑らかに更新
        for (const auto& go : m_worldObjects) {
            if (!go) continue;
            // ローカルプレイヤー自身は補間しない（自分の入力で直接動くため）
            if (localGo && go->getId() == localGo->getId()) continue;
            go->updateNetworkInterpolation(0.2f);
        }

        // ホスト起動時にローカルプレイヤーにID=1を割り当て
        if (g_network.is_host() && localGo && localGo->getId() == 0) {
            localGo->setId(1);
            std::cout << "[SceneGame] Host player assigned id=1\n";
            m_worldObjects.push_back(MakeNonOwning(localGo));
            std::cout << "[SceneGame] Host player added to worldObjects with id=1\n";
        }

        // クライアント側: サーバーから割り当てられたIDを設定
        if (!g_network.is_host() && g_network.getMyPlayerId() != 0) {
            GameObject* lg = GetLocalPlayerGameObject();
            if (lg && lg->getId() == 0) {
                lg->setId(g_network.getMyPlayerId());
                m_worldObjects.push_back(MakeNonOwning(lg));
                std::cout << "[SceneGame] Client player assigned id=" << lg->getId() << "\n";
            }
        }

        // === フレーム同期（3フレームごと） ===
        if (frameCounter % 3 == 0) {
            bool isNetworkActive = g_network.is_host() || g_network.getMyPlayerId() != 0;
            std::cout << "[Network] Frame " << frameCounter << " - NetworkActive: " << (isNetworkActive ? "YES" : "NO");

            if (isNetworkActive) {
                g_network.FrameSync(localGo, m_worldObjects);
                std::cout << " - FrameSync EXECUTED";
                if (localGo) {
                    auto pos = localGo->getPosition();
                    auto rot = localGo->getRotation();
                    std::cout << " LOCAL pos=(" << pos.x << "," << pos.y << "," << pos.z << ")"
                        << " rot=(" << rot.x << "," << rot.y << "," << rot.z << ")"
                        << " id=" << localGo->getId();

                    if (g_network.is_host()) {
                        std::cout << " [HOST] clients=" << m_worldObjects.size() - 1;
                    } else {
                        std::cout << " [CLIENT] myId=" << g_network.getMyPlayerId();
                    }
                } else {
                    std::cout << " (localGo=null)";
                }
            } else {
                std::cout << " - FrameSync SKIPPED (not connected)";
            }
            std::cout << "\n";
        }

        // === ゲームロジック更新 ===
        Engine::CollisionSystem::GetInstance().Update();
        UpdateCameraSystem();
        UpdatePlayer();

        // === 定期ステータス表示（3秒ごと） ===
        static int statusCounter = 0;
        statusCounter++;
        if (statusCounter % 180 == 0) {
            std::cout << "[NetworkStatus] Host: " << (g_network.is_host() ? "YES" : "NO")
                << " MyId: " << g_network.getMyPlayerId()
                << " WorldObjects: " << m_worldObjects.size() << "\n";
        }
    }

    void SceneGame::Draw() {
        // ===== 3D描画 =====
        if (m_pMapRenderer) {
            m_pMapRenderer->Draw();
        }

        DrawPlayers();

        // ネットワーク経由の他プレイヤーオブジェクトを描画
        GameObject* local = GetLocalPlayerGameObject();
        for (const auto& go : m_worldObjects) {
            if (!go) continue;
            if (go->getId() != 0 && go.get() != local) {
                if (local && go->getId() == local->getId()) {
                    continue;
                }
                go->draw();
            }
        }

        // ===== 2D UI描画: HP表示（画面左上） =====
        DrawHPDisplay();
    }

    // =====================================================
    // DrawHPDisplay - 画面左上にHPバーとテキストを描画
    // =====================================================
    void SceneGame::DrawHPDisplay() {
        Player* activePlayer = PlayerManager::GetInstance().GetActivePlayer();
        if (!activePlayer) return;

        int currentHP = activePlayer->GetHP();
        int maxHP = activePlayer->GetMaxHP();

        // --- HPバーの設定 ---
        const float barX = 20.0f;   // 左端からの位置
        const float barY = 20.0f;   // 上端からの位置
        const float barWidth = 200.0f;  // バーの最大幅
        const float barHeight = 20.0f;   // バーの高さ

        // HP割合に応じたバーの幅
        float hpRatio = (maxHP > 0) ? (float)currentHP / (float)maxHP : 0.0f;
        float filledWidth = barWidth * hpRatio;

        // --- HPバーの色（HPが減ると緑→黄→赤に変化） ---
        XMFLOAT4 barColor;
        if (hpRatio > 0.5f) {
            // 50%以上: 緑
            barColor = { 0.0f, 1.0f, 0.0f, 1.0f };
        } else if (hpRatio > 0.2f) {
            // 20%〜50%: 黄色
            barColor = { 1.0f, 1.0f, 0.0f, 1.0f };
        } else {
            // 20%以下: 赤
            barColor = { 1.0f, 0.0f, 0.0f, 1.0f };
        }

        // --- 描画: 背景（暗い灰色のバー） ---
        Engine::Renderer::GetInstance().DrawRect2D(
            barX, barY, barWidth, barHeight,
            { 0.3f, 0.3f, 0.3f, 0.8f }   // 半透明の暗い灰色
        );

        // --- 描画: HP残量バー ---
        if (filledWidth > 0.0f) {
            Engine::Renderer::GetInstance().DrawRect2D(
                barX, barY, filledWidth, barHeight,
                barColor
            );
        }

        // --- 描画: HPテキスト（例: "HP: 7 / 10"） ---
        char hpText[64];
        sprintf_s(hpText, "HP: %d / %d", currentHP, maxHP);
        Engine::Renderer::GetInstance().DrawText2D(
            barX + 5.0f,           // バーの少し内側
            barY + 2.0f,
            hpText,
            { 1.0f, 1.0f, 1.0f, 1.0f }  // 白色テキスト
        );
    }

    //*****************************************************************************
    // SceneTitle 実装（将来の拡張用スタブ）
    //*****************************************************************************
    SceneTitle::SceneTitle() {
        m_sceneType = SceneType::TITLE;
    }

    SceneTitle::~SceneTitle() {
    }

    HRESULT SceneTitle::Initialize() {
        return S_OK;
    }

    void SceneTitle::Finalize() {
    }

    void SceneTitle::Update() {
    }

    void SceneTitle::Draw() {
    }

    //*****************************************************************************
    // SceneResult 実装（将来の拡張用スタブ）
    //*****************************************************************************
    SceneResult::SceneResult() {
        m_sceneType = SceneType::RESULT;
    }

    SceneResult::~SceneResult() {
    }

    HRESULT SceneResult::Initialize() {
        return S_OK;
    }

    void SceneResult::Finalize() {
    }

    void SceneResult::Update() {
    }

    void SceneResult::Draw() {
    }

    //*****************************************************************************
    // Game クラス実装（シーン遷移管理）
    //*****************************************************************************
    Game::Game() {
    }

    Game::~Game() {
    }

    HRESULT Game::Initialize(SceneType firstScene) {
        m_pCurrentScene = CreateScene(firstScene);
        if (!m_pCurrentScene) {
            return E_FAIL;
        }

        m_currentSceneType = firstScene;
        return m_pCurrentScene->Initialize();
    }

    void Game::Finalize() {
        if (m_pCurrentScene) {
            m_pCurrentScene->Finalize();
            m_pCurrentScene.reset();
        }
        m_currentSceneType = SceneType::NONE;
    }

    void Game::Update() {
        if (m_sceneChangeRequested) {
            ExecuteSceneChange();
        }

        if (m_pCurrentScene) {
            m_pCurrentScene->Update();
        }
    }

    void Game::Draw() {
        if (m_pCurrentScene) {
            m_pCurrentScene->Draw();
        }
    }

    void Game::RequestSceneChange(SceneType nextScene) {
        if (nextScene != m_currentSceneType) {
            m_nextSceneType = nextScene;
            m_sceneChangeRequested = true;
        }
    }

    SceneGame* Game::GetSceneGame() const {
        if (m_currentSceneType == SceneType::GAME && m_pCurrentScene) {
            return static_cast<SceneGame*>(m_pCurrentScene.get());
        }
        return nullptr;
    }

    void Game::ExecuteSceneChange() {
        m_sceneChangeRequested = false;

        if (m_pCurrentScene) {
            m_pCurrentScene->Finalize();
            m_pCurrentScene.reset();
        }

        m_pCurrentScene = CreateScene(m_nextSceneType);
        if (m_pCurrentScene) {
            if (SUCCEEDED(m_pCurrentScene->Initialize())) {
                m_currentSceneType = m_nextSceneType;
                std::cout << "[Game] Scene changed to: " << static_cast<int>(m_currentSceneType) << "\n";
            } else {
                std::cout << "[Game] ERROR: Failed to initialize scene " << static_cast<int>(m_nextSceneType) << "\n";
                m_pCurrentScene.reset();
                m_currentSceneType = SceneType::NONE;
            }
        }

        m_nextSceneType = SceneType::NONE;
    }

    std::unique_ptr<Scene> Game::CreateScene(SceneType type) {
        switch (type) {
        case SceneType::TITLE:
            return std::make_unique<SceneTitle>();
        case SceneType::GAME:
            return std::make_unique<SceneGame>();
        case SceneType::RESULT:
            return std::make_unique<SceneResult>();
        default:
            return nullptr;
        }
    }

} // namespace Game
