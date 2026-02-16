/*********************************************************************
  \file    メイン [main.cpp]

  \Author  Ryoto Kikuchi
  \data    2025/9/26
 *********************************************************************/
#include "main.h"
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
#include <iostream>
#include <Windows.h>
#include "bullet.h"

static double g_fps = 0.0f;

// extern for player update wrapper
extern void UpdatePlayer();
extern GameObject* GetLocalPlayerGameObject();

// world objects for networking (map blocks + players)
static std::vector<GameObject*> g_worldObjects;

Player g_player;


// worldObjectsへのアクセス関数
std::vector<GameObject*>& GetWorldObjects() {
    return g_worldObjects;
}

// simple input sequence counter
static uint32_t g_inputSeq = 0;

//===================================
// ライブラリのリンク
//===================================
#pragma	comment (lib, "d3d11.lib")
#pragma	comment (lib, "d3dcompiler.lib")
#pragma	comment (lib, "winmm.lib")
#pragma	comment (lib, "dxguid.lib")
#pragma	comment (lib, "dinput8.lib")

//=================================
//マクロ定義
//=================================
#define		CLASS_NAME		"DX21 Window"
#define		WINDOW_CAPTION	"3Dtest - Player1(1キー,TPS) Player2(2キー,FPS)"

//===================================
//プロトタイプ宣言
//===================================
LRESULT	CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
HRESULT	Init(HINSTANCE hInstance, HWND hWnd, BOOL bWindow);
void	Uninit(void);
void	Update(void);
void	Draw(void);

//===================================
//グローバル変数
//===================================
static Map* g_pMap = nullptr;
static MapRenderer* g_pMapRenderer = nullptr;

//=====================================
//メイン関数
//======================================
int APIENTRY WinMain(HINSTANCE hInstance,
    HINSTANCE hPrevInstance, LPSTR lpCmd, int nCmdShow) {

    HRESULT hr = CoInitializeEx(nullptr, COINITBASE_MULTITHREADED);

    WNDCLASS	wc;
    ZeroMemory(&wc, sizeof(WNDCLASS));
    wc.lpfnWndProc = WndProc;
    wc.lpszClassName = CLASS_NAME;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_BACKGROUND + 1);
    RegisterClass(&wc);

    RECT	rc = { 0, 0, 1280, 720 };
    AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW ^ (WS_THICKFRAME | WS_MAXIMIZEBOX | WS_MINIMIZEBOX), FALSE);

    HWND	hWnd = CreateWindow(
        CLASS_NAME,
        WINDOW_CAPTION,
        WS_OVERLAPPEDWINDOW ^ (WS_THICKFRAME | WS_MAXIMIZEBOX | WS_MINIMIZEBOX),
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        rc.right - rc.left,
        rc.bottom - rc.top,
        NULL,
        NULL,
        hInstance,
        NULL
    );

    if (FAILED(Init(hInstance, hWnd, true))) {
        return -1;
    }

    SystemTimer_Initialize();

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    MSG	msg;
    ZeroMemory(&msg, sizeof(MSG));

    double exec_last_time = 0.0;
    double fps_last_time = 0.0;
    double current_time = 0.0;
    ULONG frame_count = 0;
    double fps = 0.0l;

    exec_last_time = fps_last_time = SystemTimer_GetTime();

    while (1) {
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) {
                break;
            } else {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        } else {
            current_time = SystemTimer_GetTime();
            double elapsed_time = current_time - fps_last_time;

            if (elapsed_time >= 1.0f) {
                fps = frame_count / elapsed_time;
                fps_last_time = current_time;
                frame_count = 0;
            }

            elapsed_time = current_time - exec_last_time;

            if (elapsed_time >= (1.0 / 60.0)) {
                exec_last_time = current_time;
                Update();
                Draw();
                keycopy();
                frame_count++;
            }
        }
    }

    Uninit();
    return (int)msg.wParam;
}

//=========================================
//ウィンドウプロシージャ
//=========================================
LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_ACTIVATEAPP:
    case WM_SYSKEYDOWN:
    case WM_KEYUP:
    case WM_SYSKEYUP:
        Keyboard_ProcessMessage(uMsg, wParam, lParam);
        Mouse_ProcessMessage(uMsg, wParam, lParam);
        break;
    case WM_KEYDOWN:
        if (wParam == VK_ESCAPE) {
            SendMessage(hWnd, WM_CLOSE, 0, 0);
        }
        Keyboard_ProcessMessage(uMsg, wParam, lParam);
        break;

    case WM_INPUT:
    case WM_MOUSEMOVE:
    case WM_LBUTTONDOWN:
    case WM_LBUTTONUP:
    case WM_RBUTTONDOWN:
    case WM_RBUTTONUP:
    case WM_MBUTTONDOWN:
    case WM_MBUTTONUP:
    case WM_MOUSEWHEEL:
    case WM_XBUTTONDOWN:
    case WM_XBUTTONUP:
    case WM_MOUSEHOVER:
        Mouse_ProcessMessage(uMsg, wParam, lParam);
        break;

    case WM_CLOSE:
        if (MessageBox(hWnd, "本当に終了してもよろしいですか？",
            "確認", MB_OKCANCEL | MB_DEFBUTTON2) == IDOK) {
            DestroyWindow(hWnd);
        } else {
            return 0;
        }
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    }

    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

//==================================
//初期化処理
//==================================
HRESULT	Init(HINSTANCE hInstance, HWND hWnd, BOOL bWindow) {
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
    g_pMap = new Map();
    if (g_pMap) {
        g_pMap->Initialize(Engine::GetDefaultTexture());
    }

    g_pMapRenderer = new MapRenderer();
    if (g_pMapRenderer) {
        g_pMapRenderer->Initialize(g_pMap);
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

    InitializePlayers(g_pMap, Engine::GetDefaultTexture());

    // カメラシステム初期化
    InitializeCameraSystem();

    // populate worldObjects with map blocks
    g_worldObjects.clear();
    if (g_pMap) {
        const auto& blocks = g_pMap->GetBlockObjects();
        for (const auto& up : blocks) {
            g_worldObjects.push_back(up.get());
        }
    }

    // 衝突システム初期化
    Engine::CollisionSystem::GetInstance().Initialize();
    Engine::MapCollision::GetInstance().Initialize(2.0f);

    // マップブロックをMapCollisionに登録
    if (g_pMap) {
        const auto& blocks = g_pMap->GetBlockObjects();
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
        std::cout << "[Main] Players initialized\n";
        std::cout << "[Main] 1キー: Player1 (TPS視点), 2キー: Player2 (FPS視点)\n";
    }

    return S_OK;
}

//====================================
//	終了処理
//====================================
void Uninit(void) {
    // マップ関連の終了処理
    if (g_pMapRenderer) {
        g_pMapRenderer->Uninitialize();
        delete g_pMapRenderer;
        g_pMapRenderer = nullptr;
    }

    for (auto go : g_worldObjects) {
        if (go && go->getId() != 0) {
            GameObject* local = GetLocalPlayerGameObject();
            if (go != local) delete go;
        }
    }
    g_worldObjects.clear();

    if (g_pMap) {
        g_pMap->Uninitialize();
        delete g_pMap;
        g_pMap = nullptr;
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
//更新処理
//====================================
void Update(void) {
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

    GameObject* localGo = GetLocalPlayerGameObject();
    constexpr float fixedDt = 1.0f / 60.0f;
    g_network.update(fixedDt, localGo, g_worldObjects);

    if (g_network.is_host() && localGo && localGo->getId() == 0) {
        localGo->setId(1);
        std::cout << "[Main] Host player assigned id=1\n";
        g_worldObjects.push_back(localGo);
        std::cout << "[Main] Host player added to worldObjects with id=1\n";
    }

    if (!g_network.is_host() && g_network.getMyPlayerId() != 0) {
        GameObject* lg = GetLocalPlayerGameObject();
        if (lg && lg->getId() == 0) {
            lg->setId(g_network.getMyPlayerId());
            g_worldObjects.push_back(lg);
            std::cout << "[Main] Client player assigned id=" << lg->getId() << "\n";
        }
    }

    if (frameCounter % 3 == 0) {
        bool isNetworkActive = g_network.is_host() || g_network.getMyPlayerId() != 0;
        std::cout << "[Network] Frame " << frameCounter << " - NetworkActive: " << (isNetworkActive ? "YES" : "NO");

        if (isNetworkActive) {
            g_network.FrameSync(localGo, g_worldObjects);
            std::cout << " - FrameSync EXECUTED";
            if (localGo) {
                auto pos = localGo->getPosition();
                auto rot = localGo->getRotation();
                std::cout << " LOCAL pos=(" << pos.x << "," << pos.y << "," << pos.z << ")"
                    << " rot=(" << rot.x << "," << rot.y << "," << rot.z << ")"
                    << " id=" << localGo->getId();

                if (g_network.is_host()) {
                    std::cout << " [HOST] clients=" << g_worldObjects.size() - 1;
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
            << " WorldObjects: " << g_worldObjects.size() << "\n";
    }

    static int frameCount = 0;
    static double lastFpsTime = SystemTimer_GetTime();

    frameCount++;
    double currentTime = SystemTimer_GetTime();
    double elapsed = currentTime - lastFpsTime;


    if (elapsed >= 0.5) {
        g_fps = frameCount / elapsed;
        frameCount = 0;
        lastFpsTime = currentTime;

        // ウィンドウタイトルに表示（確実に動作）
        char title[256];
        sprintf_s(title, "3Dtest - FPS: %.1f", g_fps);
        HWND hWnd = FindWindowA(CLASS_NAME, nullptr);
        if (hWnd) {
            SetWindowTextA(hWnd, title);
        }
    }
}

//==================================
//描画処理
//==================================
void Draw(void) {
    Engine::Renderer::GetInstance().Clear();

    // ===== 3D描画 =====
    if (g_pMapRenderer) {
        g_pMapRenderer->Draw();
    }

    DrawPlayers();

    GameObject* local = GetLocalPlayerGameObject();
    for (auto go : g_worldObjects) {
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
