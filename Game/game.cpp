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

namespace Game {

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
    // マップ関連の初期化
    m_pMap = new Map();
    if (m_pMap) {
        m_pMap->Initialize(Engine::GetDefaultTexture());
    }

    m_pMapRenderer = new MapRenderer();
    if (m_pMapRenderer) {
        m_pMapRenderer->Initialize(m_pMap);
    }

    // プレイヤー初期化
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

    // カメラシステム初期化
    InitializeCameraSystem();

    // populate worldObjects with map blocks
    m_worldObjects.clear();
    if (m_pMap) {
        const auto& blocks = m_pMap->GetBlockObjects();
        for (const auto& up : blocks) {
            m_worldObjects.push_back(up.get());
        }
    }

    // 衝突システム初期化
    Engine::CollisionSystem::GetInstance().Initialize();
    Engine::MapCollision::GetInstance().Initialize(2.0f);

    // マップブロックをMapCollisionに登録
    if (m_pMap) {
        const auto& blocks = m_pMap->GetBlockObjects();
        for (const auto& block : blocks) {
            Engine::MapCollision::GetInstance().RegisterBlock(block->GetBoxCollider());
        }
    }

    // 動的オブジェクト同士の衝突コールバック
    Engine::CollisionSystem::GetInstance().SetCallback(
        [](const Engine::CollisionHit& hit) {
            Bullet* bullet = nullptr;
            Player* player = nullptr;

            if (Engine::HasFlag(hit.dataA->layer, Engine::CollisionLayer::PROJECTILE))
                bullet = static_cast<Bullet*>(hit.dataA->userData);
            if (Engine::HasFlag(hit.dataB->layer, Engine::CollisionLayer::PROJECTILE))
                bullet = static_cast<Bullet*>(hit.dataB->userData);
            if (Engine::HasFlag(hit.dataA->layer, Engine::CollisionLayer::PLAYER))
                player = static_cast<Player*>(hit.dataA->userData);
            if (Engine::HasFlag(hit.dataB->layer, Engine::CollisionLayer::PLAYER))
                player = static_cast<Player*>(hit.dataB->userData);

            if (bullet && player && bullet->active && player->IsAlive()) {
                bullet->Deactivate();
                player->TakeDamage(50);
            }
        }
    );

    GameObject* localGo = GetLocalPlayerGameObject();
    if (localGo) {
        localGo->setId(0);
        std::cout << "[SceneGame] Players initialized\n";
        std::cout << "[SceneGame] 1キー: Player1 (TPS視点), 2キー: Player2 (FPS視点)\n";
    }

    return S_OK;
}

void SceneGame::Finalize() {
    // マップ関連の終了処理
    if (m_pMapRenderer) {
        m_pMapRenderer->Uninitialize();
        delete m_pMapRenderer;
        m_pMapRenderer = nullptr;
    }

    GameObject* localGo = GetLocalPlayerGameObject();
    for (auto go : m_worldObjects) {
        if (go && go->getId() != 0) {
            if (go != localGo) delete go;
        }
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

    GameObject* localGo = GetLocalPlayerGameObject();
    constexpr float fixedDt = 1.0f / 60.0f;
    g_network.update(fixedDt, localGo, m_worldObjects);

    if (g_network.is_host() && localGo && localGo->getId() == 0) {
        localGo->setId(1);
        std::cout << "[SceneGame] Host player assigned id=1\n";
        m_worldObjects.push_back(localGo);
        std::cout << "[SceneGame] Host player added to worldObjects with id=1\n";
    }

    if (!g_network.is_host() && g_network.getMyPlayerId() != 0) {
        GameObject* lg = GetLocalPlayerGameObject();
        if (lg && lg->getId() == 0) {
            lg->setId(g_network.getMyPlayerId());
            m_worldObjects.push_back(lg);
            std::cout << "[SceneGame] Client player assigned id=" << lg->getId() << "\n";
        }
    }

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

    Engine::CollisionSystem::GetInstance().Update();
    UpdateCameraSystem();
    UpdatePlayer();

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

    GameObject* local = GetLocalPlayerGameObject();
    for (auto go : m_worldObjects) {
        if (!go) continue;
        if (go->getId() != 0 && go != local) {
            if (local && go->getId() == local->getId()) {
                continue;
            }
            go->draw();
        }
    }
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
    // TODO: タイトル画面リソースの読み込み
    return S_OK;
}

void SceneTitle::Finalize() {
    // TODO: タイトル画面リソースの解放
}

void SceneTitle::Update() {
    // TODO: タイトル画面の更新処理
    // 例: ENTERキーでゲームシーンへ遷移
}

void SceneTitle::Draw() {
    // TODO: タイトル画面の描画処理
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
    // TODO: リザルト画面リソースの読み込み
    return S_OK;
}

void SceneResult::Finalize() {
    // TODO: リザルト画面リソースの解放
}

void SceneResult::Update() {
    // TODO: リザルト画面の更新処理
    // 例: ENTERキーでタイトルシーンへ遷移
}

void SceneResult::Draw() {
    // TODO: リザルト画面の描画処理
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
    // シーン遷移要求があればフレーム先頭で切り替え
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

    // 現在のシーンを終了
    if (m_pCurrentScene) {
        m_pCurrentScene->Finalize();
        m_pCurrentScene.reset();
    }

    // 新しいシーンを生成・初期化
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
