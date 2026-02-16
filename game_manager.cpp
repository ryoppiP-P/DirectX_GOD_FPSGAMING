/*********************************************************************
  \file    ゲームマネージャー [game_manager.cpp]
  
  \Author  Ryoto Kikuchi
  \data    2025
 *********************************************************************/
#include "game_manager.h"
#include "System/Core/renderer.h"
#include "System/Graphics/vertex.h"
#include "System/Graphics/material.h"
#include "System/Graphics/primitive.h"
#include "System/Graphics/sprite_2d.h"
#include "System/Graphics/sprite_3d.h"
#include "System/Collision/collision_system.h"
#include "System/Collision/map_collision.h"
#include "keyboard.h"
#include "map.h"
#include "map_renderer.h"
#include "mouse.h"
#include "player.h"
#include "camera.h"
#include "NetWork/network_manager.h"
#include "game_controller.h"
#include "system_timer.h"
#include "bullet.h"
#include "game_object.h"
#include <iostream>
#include <Windows.h>

// extern for player update wrapper
extern void UpdatePlayer();
extern GameObject* GetLocalPlayerGameObject();

//===================================
// マクロ定義
//===================================
#define		CLASS_NAME		"DX21 Window"

GameManager::GameManager() {
}

GameManager::~GameManager() {
}

GameManager& GameManager::Instance() {
    static GameManager instance;
    return instance;
}

GameObject* GameManager::GetLocalPlayerGameObject() const {
    return ::GetLocalPlayerGameObject();
}

//==================================
// 初期化処理
//==================================
HRESULT GameManager::Initialize(HINSTANCE hInstance, HWND hWnd, BOOL bWindow) {
    // DirectX関連の初期化
    auto& renderer = Engine::Renderer::GetInstance();
    if (!renderer.Initialize(hInstance, hWnd, bWindow != FALSE)) {
        return E_FAIL;
    }

    // Renderer初期化後にDebugText初期化
    if (!renderer.Initialize(hInstance, hWnd, bWindow != FALSE)) {
        return E_FAIL;
    }

    // スプライトシステム初期化（新Engine）
    Engine::Sprite2D::Initialize(renderer.GetDevice());
    Engine::Sprite3D::Initialize(renderer.GetDevice());

    Keyboard_Initialize();
    Mouse_Initialize(hWnd);
    GameController::Initialize();

    // プリミティブ初期化（新Engine）
    Engine::InitPrimitives(renderer.GetDevice());

#ifdef _DEBUG
    {
        int flags = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG);
        flags |= _CRTDBG_ALLOC_MEM_DF | _CRTDBG_CHECK_ALWAYS_DF | _CRTDBG_LEAK_CHECK_DF;
        _CrtSetDbgFlag(flags);
    }
#endif

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
    int msgRes = MessageBox(hWnd, "Choose starting player:\nYes = Player1 (TPS)\nNo = Player2 (FPS)",
        "Select Player", MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON1);
    if (msgRes == IDYES) {
        PlayerManager::GetInstance().SetInitialActivePlayer(1);
        std::cout << "[Main] Selected initial player:1\n";
    } else {
        PlayerManager::GetInstance().SetInitialActivePlayer(2);
        std::cout << "[Main] Selected initial player:2\n";
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

    GameObject* localGo = ::GetLocalPlayerGameObject();
    if (localGo) {
        localGo->setId(0);
        std::cout << "[Main] Players initialized\n";
        std::cout << "[Main] 1キー: Player1 (TPS視点), 2キー: Player2 (FPS視点)\n";
    }

    return S_OK;
}

//====================================
//	終了処理
//====================================
void GameManager::Finalize() {
    // マップ関連の終了処理
    if (m_pMapRenderer) {
        m_pMapRenderer->Uninitialize();
        delete m_pMapRenderer;
        m_pMapRenderer = nullptr;
    }

    GameObject* localGo = ::GetLocalPlayerGameObject();
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

    // スプライトシステム終了（新Engine）
    Engine::Sprite2D::Finalize();
    Engine::Sprite3D::Finalize();

    Engine::CollisionSystem::GetInstance().Shutdown();
    Engine::MapCollision::GetInstance().Shutdown();

    // プリミティブ終了（新Engine）
    Engine::UninitPrimitives();

    GameController::Shutdown();
    Mouse_Finalize();

    // DirectX関連の終了処理
    Engine::Renderer::GetInstance().Finalize();
}

//===================================
// 更新処理
//====================================
void GameManager::Update() {
    static int frameCounter = 0;
    frameCounter++;

    if (Keyboard_IsKeyDownTrigger(KK_F1)) {
        if (!g_network.is_host()) {
            if (g_network.start_as_host()) {
                std::cout << "[Main] Started as HOST - Waiting for clients...\n";
            } else {
                std::cout << "[Main] FAILED to start as HOST\n";
            }
        } else {
            std::cout << "[Main] Already running as HOST\n";
        }
    }
    if (Keyboard_IsKeyDownTrigger(KK_F2)) {
        if (!g_network.is_host()) {
            if (g_network.start_as_client()) {
                std::string hostIp;
                if (g_network.discover_and_join(hostIp)) {
                    std::cout << "[Main] Discovered host: " << hostIp << " - CLIENT CONNECTED!\n";
                } else {
                    std::cout << "[Main] CLIENT: Host discovery FAILED\n";
                }
            } else {
                std::cout << "[Main] FAILED to start as CLIENT\n";
            }
        } else {
            std::cout << "[Main] Cannot start client - already running as HOST\n";
        }
    }

    GameObject* localGo = ::GetLocalPlayerGameObject();
    constexpr float fixedDt = 1.0f / 60.0f;
    g_network.update(fixedDt, localGo, m_worldObjects);

    if (g_network.is_host() && localGo && localGo->getId() == 0) {
        localGo->setId(1);
        std::cout << "[Main] Host player assigned id=1\n";
        m_worldObjects.push_back(localGo);
        std::cout << "[Main] Host player added to worldObjects with id=1\n";
    }

    if (!g_network.is_host() && g_network.getMyPlayerId() != 0) {
        GameObject* lg = ::GetLocalPlayerGameObject();
        if (lg && lg->getId() == 0) {
            lg->setId(g_network.getMyPlayerId());
            m_worldObjects.push_back(lg);
            std::cout << "[Main] Client player assigned id=" << lg->getId() << "\n";
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

    static int frameCount = 0;
    static double lastFpsTime = SystemTimer_GetTime();

    frameCount++;
    double currentTime = SystemTimer_GetTime();
    double elapsed = currentTime - lastFpsTime;

    if (elapsed >= 0.5) {
        m_fps = frameCount / elapsed;
        frameCount = 0;
        lastFpsTime = currentTime;

        // ウィンドウタイトルに表示（確実に動く）
        char title[256];
        sprintf_s(title, "3Dtest - FPS: %.1f", m_fps);
        HWND hWnd = FindWindowA(CLASS_NAME, nullptr);
        if (hWnd) {
            SetWindowTextA(hWnd, title);
        }
    }
}

//==================================
// 描画処理
//==================================
void GameManager::Draw() {
    Engine::Renderer::GetInstance().Clear();

    // ===== 3D描画 =====
    if (m_pMapRenderer) {
        m_pMapRenderer->Draw();
    }

    DrawPlayers();

    GameObject* local = ::GetLocalPlayerGameObject();
    for (auto go : m_worldObjects) {
        if (!go) continue;
        if (go->getId() != 0 && go != local) {
            if (local && go->getId() == local->getId()) {
                continue;
            }
            go->draw();
        }
    }

    Engine::Renderer::GetInstance().Present();
}
