/*********************************************************************
  \file    メイン [main.cpp]

  \Author  Ryoto Kikuchi
  \data    2025/9/26
 *********************************************************************/
#include "main.h"
#include "renderer.h"
#include "polygon.h"
#include "sprite.h"
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
//コールバック関数（定義した名前で呼び出される関数）
LRESULT	CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

//初期化関数
HRESULT	Init(HINSTANCE hInstance, HWND hWnd, BOOL bWindow);
//終了処理
void	Uninit(void);
//更新処理
void	Update(void);
//描画処理
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

    //ウィンドウクラスの登録（ウィンドウの仕様的な部分を決めてWindowsにセットする）
    WNDCLASS	wc;	//構造体宣言
    ZeroMemory(&wc, sizeof(WNDCLASS));//毎度０で初期化
    wc.lpfnWndProc = WndProc;	//コールバック関数のポインター
    wc.lpszClassName = CLASS_NAME;	//この仕様書の名前
    wc.hInstance = hInstance;	//このアプリケーションのもの
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);//カーソルの種類
    wc.hbrBackground = (HBRUSH)(COLOR_BACKGROUND + 1);//ウィンドウの背景色
    RegisterClass(&wc);	//構造体をWindowsへセット


    //ウィンドウサイズの調整
    //   横幅　　縦幅
    RECT	rc = { 0, 0, 1280, 720 };//横1280 縦720
    //描画の域が1280X720になるようにサイズを調整する
    AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW ^ (WS_THICKFRAME | WS_MAXIMIZEBOX | WS_MINIMIZEBOX), FALSE);

    //ウィンドウの作成
    HWND	hWnd = CreateWindow(
        CLASS_NAME,	//作りたいウィンドウ
        WINDOW_CAPTION,	//ウィンドウに表示されるタイトル
        WS_OVERLAPPEDWINDOW ^ (WS_THICKFRAME | WS_MAXIMIZEBOX | WS_MINIMIZEBOX),	//標準的な形のウィンドウ サイズ変更禁止
        CW_USEDEFAULT,		//デフォルト設定でお任せ
        CW_USEDEFAULT,
        rc.right - rc.left,//CW_USEDEFAULT,//ウィンドウの幅
        rc.bottom - rc.top,//CW_USEDEFAULT,//ウィンドウの高さ
        NULL,
        NULL,
        hInstance,		//アプリケーションのハンドル
        NULL
    );

    //初期化処理
    if (FAILED(Init(hInstance, hWnd, true))) {
        return -1;//初期化処理失敗
    }

    SystemTimer_Initialize(); // システムタイマー初期化

    //作成したウィンドウを表示
    ShowWindow(hWnd, nCmdShow);//引数に従って表示、または非表示
    //ウィンドウの内容を最新表示
    UpdateWindow(hWnd);

    //メッセージループ
    MSG	msg;
    ZeroMemory(&msg, sizeof(MSG));//メッセージ構造体を作成して初期化


    double exec_last_time = 0.0;    // 前回のゲーム処理実行時刻
    double fps_last_time = 0.0;     // 前回のFPS計算時刻  
    double current_time = 0.0;      // 現在時刻
    ULONG frame_count = 0;          // フレームカウンタ
    double fps = 0.0l;              // 現在のFPS値

    exec_last_time = fps_last_time = SystemTimer_GetTime();

    //終了メッセージが来るまでループする
    //ゲームループ
    while (1) {	//メッセージの有無をチェック
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) { //Windowsからメッセージが届いた
            if (msg.message == WM_QUIT)//完全終了しますメッセージ
            {
                break;	//whileループから脱出する
            } else {
                TranslateMessage(&msg);
                DispatchMessage(&msg);	//WndProcを呼び出させる
            }

        } else //Windowsからメッセージが来ていない
        {

            // 現在時刻を取得
            current_time = SystemTimer_GetTime();

            // FPS計算用の経過時間計算
            double elapsed_time = current_time - fps_last_time;

            // 1秒経過したらFPSを計算
            if (elapsed_time >= 1.0f) {
                fps = frame_count / elapsed_time;
                fps_last_time = current_time;
                frame_count = 0;
            }

            // ゲーム処理用の経過時間計算
            elapsed_time = current_time - exec_last_time;

            if (elapsed_time >= (1.0 / 60.0)) {

                exec_last_time = current_time;  // 実行時刻を更新

                Update();	//更新処理
                Draw();		//描画処理
                keycopy();

                frame_count++;  // フレームカウンタを増加
            }
        }

    }//while

    //終了処理
    Uninit();

    //終了処理
    return (int)msg.wParam;

}

//=========================================
//ウィンドウプロシージャ
// メッセージループから呼び出される
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
    case WM_KEYDOWN:	//キーが押された
        if (wParam == VK_ESCAPE)//押されたのがESCキー
        {
            //ウィンドウを閉じるリクエストをWindowsに送る
            SendMessage(hWnd, WM_CLOSE, 0, 0);
        }
        Keyboard_ProcessMessage(uMsg, wParam, lParam);
        break;

        // マウスメッセージの追加
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

    case WM_CLOSE:	// ウィンドウ終了
        if (
            MessageBox(hWnd, "本当に終了してもよろしいですか？",
                "確認", MB_OKCANCEL | MB_DEFBUTTON2) == IDOK
            ) {//OKが押されたとき
            DestroyWindow(hWnd);//終了処理続行をWindowsにリクエスト
        } else {
            return 0;	//戻り値０＝終了しない
        }

        break;
    case WM_DESTROY:	//終了処理OKです
        PostQuitMessage(0);		//０番のメッセージに０を送る
        break;

    }

    //該当の無いメッセージは適当に処理して終了
    return DefWindowProc(hWnd, uMsg, wParam, lParam);

}

//==================================
//初期化処理
//==================================
HRESULT	Init(HINSTANCE hInstance, HWND hWnd, BOOL bWindow) {
    //DirectX関連の初期化
    InitRenderer(hInstance, hWnd, bWindow);
    InitSprite();

    Keyboard_Initialize();
    Mouse_Initialize(hWnd);
    GameController::Initialize();

    InitPolygon();//ポリゴン表示サンプルの初期化

    // Enable CRT debug heap checks early in debug builds
#ifdef _DEBUG
    {
        int flags = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG);
        flags |= _CRTDBG_ALLOC_MEM_DF | _CRTDBG_CHECK_ALWAYS_DF | _CRTDBG_LEAK_CHECK_DF;
        _CrtSetDbgFlag(flags);
    }
#endif

    //マップ関連の初期化
    g_pMap = new Map();
    if (g_pMap) {
        g_pMap->Initialize(GetPolygonTexture());
    }

    g_pMapRenderer = new MapRenderer();
    if (g_pMapRenderer) {
        g_pMapRenderer->Initialize(g_pMap);
    }

    // プレイヤー初期化（新システム）
    extern ID3D11ShaderResourceView* GetPolygonTexture(); // polygon.cppで定義

    // Ask user which player to use at game start. Lock selection for whole run.
    int msgRes = MessageBox(hWnd, "Choose starting player:\nYes = Player1 (TPS)\nNo = Player2 (FPS)", "Select Player", MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON1);
    if (msgRes == IDYES) {
        PlayerManager::GetInstance().SetInitialActivePlayer(1);
        std::cout << "[Main] Selected initial player:1\n";
    } else {
        PlayerManager::GetInstance().SetInitialActivePlayer(2);
        std::cout << "[Main] Selected initial player:2\n";
    }

    InitializePlayers(g_pMap, GetPolygonTexture());

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

    // ローカルプレイヤーをworldObjectsに追加（ホスト側では後でネットワーク処理で管理される）
    GameObject* localGo = GetLocalPlayerGameObject();
    if (localGo) {
        localGo->setId(0); // 仮想番号で ID=0（後で割り当て）
        std::cout << "[Main] Players initialized\n";
        std::cout << "[Main] 1キー: Player1 (TPS視点), 2キー: Player2 (FPS視点)\n";
    }

    return	S_OK;
}

//====================================
//	終了処理
//====================================
void	Uninit(void) {

    //マップ関連の終了処理
    if (g_pMapRenderer) {
        g_pMapRenderer->Uninitialize();
        delete g_pMapRenderer;
        g_pMapRenderer = nullptr;
    }

    // free any dynamically created network objects (simple cleanup)
    for (auto go : g_worldObjects) {
        // map block pointers are owned elsewhere; only delete those with non-zero id (remote players created by network)
        if (go && go->getId() != 0) {
            // avoid deleting local player
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

    UninitSprite();
    UninitPolygon();//ポリゴン表示サンプル終了処理

    GameController::Shutdown();
    Mouse_Finalize();

    //DirectX関連の終了処理
    UninitRenderer();
}

//===================================
//更新処理
//====================================
void	Update(void) {
    // フレームカウンター（10フレームに1回の同期用）
    static int frameCounter = 0;
    frameCounter++;

    // host/client toggle: F1 = host, F2 = client (one-time trigger)
    if (Keyboard_IsKeyDownTrigger(KK_F1)) {
        if (!g_network.is_host()) {
            if (g_network.start_as_host()) {
                // host started
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

    // network update (pass local player's GameObject)
    GameObject* localGo = GetLocalPlayerGameObject();
    constexpr float fixedDt = 1.0f / 60.0f;
    g_network.update(fixedDt, localGo, g_worldObjects);

    // ホスト側：自分のプレイヤーIDを設定し、worldObjectsに自分用のGameObjectを追加
    if (g_network.is_host() && localGo && localGo->getId() == 0) {
        // ホストのプレイヤーIDは通常1
        localGo->setId(1);
        std::cout << "[Main] Host player assigned id=1\n";

        // ホスト用のネットワークGameObjectは作成しない（ローカルプレイヤーをworldObjectsに直接追加）
        g_worldObjects.push_back(localGo);
        std::cout << "[Main] Host player added to worldObjects with id=1\n";
    }

    // クライアント側：プレイヤーIDが割り当てられたらworldObjectsに追加
    if (!g_network.is_host() && g_network.getMyPlayerId() != 0) {
        GameObject* lg = GetLocalPlayerGameObject();
        if (lg && lg->getId() == 0) {
            lg->setId(g_network.getMyPlayerId());
            g_worldObjects.push_back(lg); // クライアント側はローカルプレイヤーをworldObjectsに追加
            std::cout << "[Main] Client player assigned id=" << lg->getId() << "\n";
        }
    }

    // *** 10フレームに1回の位置同期 (60FPS / 10 = 6Hz同期) ***
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
                    std::cout << " [HOST] clients=" << g_worldObjects.size() - 1;  // -1 for map blocks
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

    // カメラシステムの更新
    UpdateCameraSystem();
    // プレイヤーの更新
    UpdatePlayer();

    // *** ネットワーク状態の定期表示 ***
    static int statusCounter = 0;
    statusCounter++;
    if (statusCounter % 180 == 0) { // 3秒に1回
        std::cout << "[NetworkStatus] Host: " << (g_network.is_host() ? "YES" : "NO")
            << " MyId: " << g_network.getMyPlayerId()
            << " WorldObjects: " << g_worldObjects.size() << "\n";
    }

    //// *** 追加 ***
    //constexpr float dt = 1.0f / 60.0f;
    //g_npcManager.Update(dt, &g_player);
    //g_npcManager.CheckPlayerCollisions(&g_player);
}

//==================================
//描画処理
//==================================
void	Draw(void) {
    //バックバッファのクリア
    Clear();

    //マップ描画
    if (g_pMapRenderer) {
        g_pMapRenderer->Draw();
    }

    // プレイヤー描画（新システム）
    DrawPlayers(); // 両方のプレイヤーを描画

    // draw remote network objects (players)
    GameObject* local = GetLocalPlayerGameObject();
    for (auto go : g_worldObjects) {
        if (!go) continue;
        // skip map blocks (id==0) and skip local player's own object
        if (go->getId() != 0 && go != local) {
            // ネットワークオブジェクトを描画（ホスト・クライアント問わず）
                   // 自分のローカルプレイヤーと同じIDのオブジェクトは描画しない
            if (local && go->getId() == local->getId()) {
                continue; // 自分のプレイヤーと同じIDのネットワークオブジェクトはスキップ
            }

            go->draw();

            // デバッグ：描画したオブジェクトの情報出力
            static int debugCounter = 0;
            if (debugCounter % 60 == 0) { // 1秒に1回
                auto pos = go->getPosition();
                std::cout << "[Draw] Remote player id=" << go->getId()
                    << " pos=(" << pos.x << "," << pos.y << "," << pos.z << ")\n";
            }
            debugCounter++;
        }
    }

    //バックバッファをフロントバッファにコピー
    Present();
}