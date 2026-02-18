/*********************************************************************
  \file    ゲームマネージャー [game_manager.cpp]
  
  \Author  Ryoto Kikuchi
  \data    2025
 *********************************************************************/
#include "pch.h"
#include "game_manager.h"
#include "Engine/Core/renderer.h"
#include "Engine/Core/timer.h"
#include "Game/Objects/game_object.h"
#include "Game/Objects/camera.h"
#include <iostream>
#include <Windows.h>

//===================================
// マクロ定義
//===================================
#define		CLASS_NAME		"DX21 Window"

// フォールバック用の空ワールドオブジェクトリスト
std::vector<GameObject*> GameManager::s_emptyWorldObjects;

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

std::vector<GameObject*>& GameManager::GetWorldObjects() {
    SceneGame* sg = m_game.GetSceneGame();
    if (sg) return sg->GetWorldObjects();
    return s_emptyWorldObjects;
}

const std::vector<GameObject*>& GameManager::GetWorldObjects() const {
    const SceneGame* sg = m_game.GetSceneGame();
    if (sg) return sg->GetWorldObjects();
    return s_emptyWorldObjects;
}

Map* GameManager::GetMap() const {
    const SceneGame* sg = m_game.GetSceneGame();
    if (sg) return sg->GetMap();
    return nullptr;
}

MapRenderer* GameManager::GetMapRenderer() const {
    const SceneGame* sg = m_game.GetSceneGame();
    if (sg) return sg->GetMapRenderer();
    return nullptr;
}

//==================================
// 初期化処理
//==================================
HRESULT GameManager::Initialize() {

#ifdef _DEBUG
    {
        int flags = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG);
        flags |= _CRTDBG_ALLOC_MEM_DF | _CRTDBG_CHECK_ALWAYS_DF | _CRTDBG_LEAK_CHECK_DF;
        _CrtSetDbgFlag(flags);
    }
#endif

    // シーン初期化（ゲームシーンから開始）
    if (FAILED(m_game.Initialize(SceneType::GAME))) {
        return E_FAIL;
    }

    return S_OK;
}

//====================================
//	終了処理
//====================================
void GameManager::Finalize() {
    // シーン終了
    m_game.Finalize();
}

//===================================
// 更新処理
//====================================
void GameManager::Update() {
    // シーンの更新
    m_game.Update();

    // FPS計測
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

    // 現在のシーンの描画
    m_game.Draw();

    Engine::Renderer::GetInstance().Present();
}
