/*********************************************************************
  \file    エンジンシステム統括 [system.cpp]
  
  \Author  Ryoto Kikuchi
  \data    2025
 *********************************************************************/
#include "pch.h"
#include "system.h"

#include "Engine/Core/renderer.h"
#include "Engine/Core/timer.h"
#include "Engine/Input/input_manager.h"
#include "Engine/Collision/collision_manager.h"
#include "Engine/Graphics/sprite_2d.h"
#include "Engine/Graphics/sprite_3d.h"
#include "Engine/Graphics/primitive.h"

namespace Engine {

// シングルトンインスタンス取得
System& System::GetInstance() {
    static System instance;
    return instance;
}

// システム初期化
bool System::Initialize(HINSTANCE hInstance, HWND hWnd, bool windowed) {
    // 1. Timer
    SystemTimer_Initialize();

    // 2. Renderer
    if (!Renderer::GetInstance().Initialize(hInstance, hWnd, windowed)) {
        return false;
    }

    // 3. InputManager (内部でGameController初期化)
    InputManager::GetInstance().Initialize(hWnd);

    // 4. CollisionManager
    CollisionManager::GetInstance().Initialize();

    // 5. グラフィックスサブシステム（Sprite, Primitive）
    auto* pDevice = Renderer::GetInstance().GetDevice();
    Sprite2D::Initialize(pDevice);
    Sprite3D::Initialize(pDevice);
    InitPrimitives(pDevice);

    m_lastTime = SystemTimer_GetTime();
    m_initialized = true;
    return true;
}

// システム終了処理
void System::Finalize() {
    if (!m_initialized) return;

    // グラフィックスサブシステム終了
    UninitPrimitives();
    Sprite3D::Finalize();
    Sprite2D::Finalize();

    CollisionManager::GetInstance().Shutdown();
    InputManager::GetInstance().Finalize();
    Renderer::GetInstance().Finalize();

    m_initialized = false;
}

// フレーム更新
void System::Update() {
    double currentTime = SystemTimer_GetTime();
    m_deltaTime = static_cast<float>(currentTime - m_lastTime);
    m_lastTime = currentTime;

    InputManager::GetInstance().Update();
}

// サブシステムアクセサ
Renderer& System::GetRenderer() { return Renderer::GetInstance(); }
InputManager& System::GetInput() { return InputManager::GetInstance(); }
CollisionManager& System::GetCollision() { return CollisionManager::GetInstance(); }

// 便利関数
System& GetSystem() { return System::GetInstance(); }
float GetDeltaTime() { return GetSystem().GetDeltaTime(); }

} // namespace Engine
