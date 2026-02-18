/*********************************************************************
  \file    エンジンシステム統括 [system.h]
  
  \Author  Ryoto Kikuchi
  \data    2025
 *********************************************************************/
#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>

namespace Engine {

// 前方宣言
class Renderer;
class InputManager;
class CollisionManager;

class System {
private:
    bool m_initialized = false;
    float m_deltaTime = 0.0f;
    double m_lastTime = 0.0;

    System() = default;

public:
    static System& GetInstance();

    bool Initialize(HINSTANCE hInstance, HWND hWnd, bool windowed);
    void Finalize();
    void Update();

    Renderer& GetRenderer();
    InputManager& GetInput();
    CollisionManager& GetCollision();

    float GetDeltaTime() const { return m_deltaTime; }
    bool IsInitialized() const { return m_initialized; }

    System(const System&) = delete;
    System& operator=(const System&) = delete;
};

// 便利関数
System& GetSystem();
float GetDeltaTime();

} // namespace Engine
