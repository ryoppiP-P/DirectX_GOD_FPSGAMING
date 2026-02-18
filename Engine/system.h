/*********************************************************************
  \file    エンジンシステム統括 [system.h]
  
  \Author  Ryoto Kikuchi
  \data    2025
 *********************************************************************/
#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include "Engine/Core/renderer.h"
#include "Engine/Core/timer.h"
#include "Engine/Input/input_manager.h"
#include "Engine/Collision/collision_manager.h"
#include "Engine/Graphics/sprite_2d.h"
#include "Engine/Graphics/sprite_3d.h"
#include "Engine/Graphics/primitive.h"

namespace Engine {

class System {
private:
    bool m_initialized = false;
    float m_deltaTime = 0.0f;
    double m_lastTime = 0.0;

    System() = default;

public:
    static System& GetInstance() {
        static System instance;
        return instance;
    }

    bool Initialize(HINSTANCE hInstance, HWND hWnd, bool windowed) {
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

    void Finalize() {
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

    void Update() {
        double currentTime = SystemTimer_GetTime();
        m_deltaTime = static_cast<float>(currentTime - m_lastTime);
        m_lastTime = currentTime;

        InputManager::GetInstance().Update();
    }

    Renderer& GetRenderer() { return Renderer::GetInstance(); }
    InputManager& GetInput() { return InputManager::GetInstance(); }
    CollisionManager& GetCollision() { return CollisionManager::GetInstance(); }

    float GetDeltaTime() const { return m_deltaTime; }
    bool IsInitialized() const { return m_initialized; }

    System(const System&) = delete;
    System& operator=(const System&) = delete;
};

// 便利関数
inline System& GetSystem() { return System::GetInstance(); }
inline float GetDeltaTime() { return GetSystem().GetDeltaTime(); }

} // namespace Engine
