/*********************************************************************
  \file    ���C�� [main.cpp]

  \Author  Ryoto Kikuchi
  \data    2025/9/26
 *********************************************************************/
#include "main.h"
#include "System/Core/renderer.h"
#include "System/Graphics/vertex.h"
#include "System/Graphics/material.h"
#include "System/Graphics/primitive.h"
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


// worldObjects�ւ̃A�N�Z�X�֐�
std::vector<GameObject*>& GetWorldObjects() {
    return g_worldObjects;
}

// simple input sequence counter
static uint32_t g_inputSeq = 0;

//===================================
// ���C�u�����̃����N
//===================================
#pragma	comment (lib, "d3d11.lib")
#pragma	comment (lib, "d3dcompiler.lib")
#pragma	comment (lib, "winmm.lib")
#pragma	comment (lib, "dxguid.lib")
#pragma	comment (lib, "dinput8.lib")

//=================================
//�}�N����`
//=================================
#define		CLASS_NAME		"DX21 Window"
#define		WINDOW_CAPTION	"3Dtest - Player1(1�L�[,TPS) Player2(2�L�[,FPS)"

//===================================
//�v���g�^�C�v�錾
//===================================
//�R�[���o�b�N�֐��i��`�������O�ŌĂяo�����֐��j
LRESULT	CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

//�������֐�
HRESULT	Init(HINSTANCE hInstance, HWND hWnd, BOOL bWindow);
//�I������
void	Uninit(void);
//�X�V����
void	Update(void);
//�`�揈��
void	Draw(void);

//===================================
//�O���[�o���ϐ�
//===================================
static Map* g_pMap = nullptr;
static MapRenderer* g_pMapRenderer = nullptr;

//=====================================
//���C���֐�
//======================================
int APIENTRY WinMain(HINSTANCE hInstance,
    HINSTANCE hPrevInstance, LPSTR lpCmd, int nCmdShow) {

    HRESULT hr = CoInitializeEx(nullptr, COINITBASE_MULTITHREADED);

    //�E�B���h�E�N���X�̓o�^�i�E�B���h�E�̎d�l�I�ȕ��������߂�Windows�ɃZ�b�g����j
    WNDCLASS	wc;	//�\���̐錾
    ZeroMemory(&wc, sizeof(WNDCLASS));//���x�O�ŏ�����
    wc.lpfnWndProc = WndProc;	//�R�[���o�b�N�֐��̃|�C���^�[
    wc.lpszClassName = CLASS_NAME;	//���̎d�l���̖��O
    wc.hInstance = hInstance;	//���̃A�v���P�[�V�����̂���
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);//�J�[�\���̎��
    wc.hbrBackground = (HBRUSH)(COLOR_BACKGROUND + 1);//�E�B���h�E�̔w�i�F
    RegisterClass(&wc);	//�\���̂�Windows�փZ�b�g


    //�E�B���h�E�T�C�Y�̒���
    //   �����@�@�c��
    RECT	rc = { 0, 0, 1280, 720 };//��1280 �c720
    //�`��̈悪1280X720�ɂȂ�悤�ɃT�C�Y�𒲐�����
    AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW ^ (WS_THICKFRAME | WS_MAXIMIZEBOX | WS_MINIMIZEBOX), FALSE);

    //�E�B���h�E�̍쐬
    HWND	hWnd = CreateWindow(
        CLASS_NAME,	//��肽���E�B���h�E
        WINDOW_CAPTION,	//�E�B���h�E�ɕ\�������^�C�g��
        WS_OVERLAPPEDWINDOW ^ (WS_THICKFRAME | WS_MAXIMIZEBOX | WS_MINIMIZEBOX),	//�W���I�Ȍ`�̃E�B���h�E �T�C�Y�ύX�֎~
        CW_USEDEFAULT,		//�f�t�H���g�ݒ�ł��C��
        CW_USEDEFAULT,
        rc.right - rc.left,//CW_USEDEFAULT,//�E�B���h�E�̕�
        rc.bottom - rc.top,//CW_USEDEFAULT,//�E�B���h�E�̍���
        NULL,
        NULL,
        hInstance,		//�A�v���P�[�V�����̃n���h��
        NULL
    );

    //����������
    if (FAILED(Init(hInstance, hWnd, true))) {
        return -1;//�������������s
    }

    SystemTimer_Initialize(); // �V�X�e���^�C�}�[������

    //�쐬�����E�B���h�E��\��
    ShowWindow(hWnd, nCmdShow);//�����ɏ]���ĕ\���A�܂��͔�\��
    //�E�B���h�E�̓��e���ŐV�\��
    UpdateWindow(hWnd);

    //���b�Z�[�W���[�v
    MSG	msg;
    ZeroMemory(&msg, sizeof(MSG));//���b�Z�[�W�\���̂��쐬���ď�����


    double exec_last_time = 0.0;    // �O��̃Q�[���������s����
    double fps_last_time = 0.0;     // �O���FPS�v�Z����  
    double current_time = 0.0;      // ���ݎ���
    ULONG frame_count = 0;          // �t���[���J�E���^
    double fps = 0.0l;              // ���݂�FPS�l

    exec_last_time = fps_last_time = SystemTimer_GetTime();

    //�I�����b�Z�[�W������܂Ń��[�v����
    //�Q�[�����[�v
    while (1) {	//���b�Z�[�W�̗L�����`�F�b�N
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) { //Windows���烁�b�Z�[�W���͂���
            if (msg.message == WM_QUIT)//���S�I�����܂����b�Z�[�W
            {
                break;	//while���[�v����E�o����
            } else {
                TranslateMessage(&msg);
                DispatchMessage(&msg);	//WndProc���Ăяo������
            }

        } else //Windows���烁�b�Z�[�W�����Ă��Ȃ�
        {

            // ���ݎ������擾
            current_time = SystemTimer_GetTime();

            // FPS�v�Z�p�̌o�ߎ��Ԍv�Z
            double elapsed_time = current_time - fps_last_time;

            // 1�b�o�߂�����FPS���v�Z
            if (elapsed_time >= 1.0f) {
                fps = frame_count / elapsed_time;
                fps_last_time = current_time;
                frame_count = 0;
            }

            // �Q�[�������p�̌o�ߎ��Ԍv�Z
            elapsed_time = current_time - exec_last_time;

            if (elapsed_time >= (1.0 / 60.0)) {

                exec_last_time = current_time;  // ���s�������X�V

                Update();	//�X�V����
                Draw();		//�`�揈��
                keycopy();

                frame_count++;  // �t���[���J�E���^�𑝉�
            }
        }

    }//while

    //�I������
    Uninit();

    //�I������
    return (int)msg.wParam;

}

//=========================================
//�E�B���h�E�v���V�[�W��
// ���b�Z�[�W���[�v����Ăяo�����
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
    case WM_KEYDOWN:	//�L�[�������ꂽ
        if (wParam == VK_ESCAPE)//�����ꂽ�̂�ESC�L�[
        {
            //�E�B���h�E����郊�N�G�X�g��Windows�ɑ���
            SendMessage(hWnd, WM_CLOSE, 0, 0);
        }
        Keyboard_ProcessMessage(uMsg, wParam, lParam);
        break;

        // �}�E�X���b�Z�[�W�̒ǉ�
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

    case WM_CLOSE:	// �E�B���h�E�I��
        if (
            MessageBox(hWnd, "�{���ɏI�����Ă���낵���ł����H",
                "�m�F", MB_OKCANCEL | MB_DEFBUTTON2) == IDOK
            ) {//OK�������ꂽ�Ƃ�
            DestroyWindow(hWnd);//�I���������s��Windows�Ƀ��N�G�X�g
        } else {
            return 0;	//�߂�l�O���I�����Ȃ�
        }

        break;
    case WM_DESTROY:	//�I������OK�ł�
        PostQuitMessage(0);		//�O�Ԃ̃��b�Z�[�W�ɂO�𑗂�
        break;

    }

    //�Y���̖������b�Z�[�W�͓K���ɏ������ďI��
    return DefWindowProc(hWnd, uMsg, wParam, lParam);

}

//==================================
//����������
//==================================
HRESULT	Init(HINSTANCE hInstance, HWND hWnd, BOOL bWindow) {
    //DirectX�֘A�̏�����
    Engine::Renderer::GetInstance().Initialize(hInstance, hWnd, bWindow != FALSE);
    InitSprite();

    Keyboard_Initialize();
    Mouse_Initialize(hWnd);
    GameController::Initialize();

    InitPolygon();//�|���S���\���T���v���̏�����

    // Enable CRT debug heap checks early in debug builds
#ifdef _DEBUG
    {
        int flags = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG);
        flags |= _CRTDBG_ALLOC_MEM_DF | _CRTDBG_CHECK_ALWAYS_DF | _CRTDBG_LEAK_CHECK_DF;
        _CrtSetDbgFlag(flags);
    }
#endif

    //�}�b�v�֘A�̏�����
    g_pMap = new Map();
    if (g_pMap) {
        g_pMap->Initialize(GetPolygonTexture());
    }

    g_pMapRenderer = new MapRenderer();
    if (g_pMapRenderer) {
        g_pMapRenderer->Initialize(g_pMap);
    }

    // �v���C���[�������i�V�V�X�e���j

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

    // �J�����V�X�e��������
    InitializeCameraSystem();

    // populate worldObjects with map blocks
    g_worldObjects.clear();
    if (g_pMap) {
        const auto& blocks = g_pMap->GetBlockObjects();
        for (const auto& up : blocks) {
            g_worldObjects.push_back(up.get());
        }
    }

    // ���[�J���v���C���[��worldObjects�ɒǉ��i�z�X�g���ł͌�Ńl�b�g���[�N�����ŊǗ������j
    GameObject* localGo = GetLocalPlayerGameObject();
    if (localGo) {
        localGo->setId(0); // ���z�ԍ��� ID=0�i��Ŋ��蓖�āj
        std::cout << "[Main] Players initialized\n";
        std::cout << "[Main] 1�L�[: Player1 (TPS���_), 2�L�[: Player2 (FPS���_)\n";
    }

    return	S_OK;
}

//====================================
//	�I������
//====================================
void	Uninit(void) {

    //�}�b�v�֘A�̏I������
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
    UninitPolygon();//�|���S���\���T���v���I������

    GameController::Shutdown();
    Mouse_Finalize();

    //DirectX�֘A�̏I������
    Engine::Renderer::GetInstance().Finalize();
}

//===================================
//�X�V����
//====================================
void	Update(void) {
    // �t���[���J�E���^�[�i10�t���[����1��̓����p�j
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

    // �z�X�g���F�����̃v���C���[ID��ݒ肵�AworldObjects�Ɏ����p��GameObject��ǉ�
    if (g_network.is_host() && localGo && localGo->getId() == 0) {
        // �z�X�g�̃v���C���[ID�͒ʏ�1
        localGo->setId(1);
        std::cout << "[Main] Host player assigned id=1\n";

        // �z�X�g�p�̃l�b�g���[�NGameObject�͍쐬���Ȃ��i���[�J���v���C���[��worldObjects�ɒ��ڒǉ��j
        g_worldObjects.push_back(localGo);
        std::cout << "[Main] Host player added to worldObjects with id=1\n";
    }

    // �N���C�A���g���F�v���C���[ID�����蓖�Ă�ꂽ��worldObjects�ɒǉ�
    if (!g_network.is_host() && g_network.getMyPlayerId() != 0) {
        GameObject* lg = GetLocalPlayerGameObject();
        if (lg && lg->getId() == 0) {
            lg->setId(g_network.getMyPlayerId());
            g_worldObjects.push_back(lg); // �N���C�A���g���̓��[�J���v���C���[��worldObjects�ɒǉ�
            std::cout << "[Main] Client player assigned id=" << lg->getId() << "\n";
        }
    }

    // *** 10�t���[����1��̈ʒu���� (60FPS / 10 = 6Hz����) ***
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

    // �J�����V�X�e���̍X�V
    UpdateCameraSystem();
    // �v���C���[�̍X�V
    UpdatePlayer();

    // *** �l�b�g���[�N��Ԃ̒���\�� ***
    static int statusCounter = 0;
    statusCounter++;
    if (statusCounter % 180 == 0) { // 3�b��1��
        std::cout << "[NetworkStatus] Host: " << (g_network.is_host() ? "YES" : "NO")
            << " MyId: " << g_network.getMyPlayerId()
            << " WorldObjects: " << g_worldObjects.size() << "\n";
    }

    //// *** �ǉ� ***
    //constexpr float dt = 1.0f / 60.0f;
    //g_npcManager.Update(dt, &g_player);
    //g_npcManager.CheckPlayerCollisions(&g_player);
}

//==================================
//�`�揈��
//==================================
void	Draw(void) {
    //�o�b�N�o�b�t�@�̃N���A
    Engine::Renderer::GetInstance().Clear();

    //�}�b�v�`��
    if (g_pMapRenderer) {
        g_pMapRenderer->Draw();
    }

    // �v���C���[�`��i�V�V�X�e���j
    DrawPlayers(); // �����̃v���C���[��`��

    // draw remote network objects (players)
    GameObject* local = GetLocalPlayerGameObject();
    for (auto go : g_worldObjects) {
        if (!go) continue;
        // skip map blocks (id==0) and skip local player's own object
        if (go->getId() != 0 && go != local) {
            // �l�b�g���[�N�I�u�W�F�N�g��`��i�z�X�g�E�N���C�A���g��킸�j
                   // �����̃��[�J���v���C���[�Ɠ���ID�̃I�u�W�F�N�g�͕`�悵�Ȃ�
            if (local && go->getId() == local->getId()) {
                continue; // �����̃v���C���[�Ɠ���ID�̃l�b�g���[�N�I�u�W�F�N�g�̓X�L�b�v
            }

            go->draw();

            // �f�o�b�O�F�`�悵���I�u�W�F�N�g�̏��o��
            static int debugCounter = 0;
            if (debugCounter % 60 == 0) { // 1�b��1��
                auto pos = go->getPosition();
                std::cout << "[Draw] Remote player id=" << go->getId()
                    << " pos=(" << pos.x << "," << pos.y << "," << pos.z << ")\n";
            }
            debugCounter++;
        }
    }

    //�o�b�N�o�b�t�@���t�����g�o�b�t�@�ɃR�s�[
    Engine::Renderer::GetInstance().Present();
}