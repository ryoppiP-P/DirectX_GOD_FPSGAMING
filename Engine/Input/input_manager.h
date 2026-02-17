/*********************************************************************
  \file    統合入力マネージャー [input_manager.h]
  
  \Author  Ryoto Kikuchi
  \data    2025
 *********************************************************************/
#pragma once

#include "Engine/Input/keyboard.h"
#include "Engine/Input/mouse.h"
#include "Engine/Input/game_controller.h"
#include <DirectXMath.h>

namespace Engine {

struct FPSCommand {
    float moveForward = 0.0f;   // W/S or 左スティックY (-1.0 ~ 1.0)
    float moveRight = 0.0f;     // A/D or 左スティックX (-1.0 ~ 1.0)
    float lookX = 0.0f;         // マウスX移動量 or 右スティックX
    float lookY = 0.0f;         // マウスY移動量 or 右スティックY

    bool jump = false;
    bool sprint = false;
    bool crouch = false;
    bool fire = false;
    bool aim = false;
    bool reload = false;
    bool interact = false;
    bool melee = false;

    bool jumpTrigger = false;
    bool fireTrigger = false;
    bool aimTrigger = false;
    bool reloadTrigger = false;

    bool weaponNext = false;
    bool weaponPrev = false;
    bool weapon1 = false;
    bool weapon2 = false;
    bool weapon3 = false;
    bool weapon4 = false;

    bool pause = false;
    bool pauseTrigger = false;
    bool scoreboard = false;

    DirectX::XMFLOAT2 mousePosition = { 0.0f, 0.0f };
    DirectX::XMFLOAT2 leftStick = { 0.0f, 0.0f };
    DirectX::XMFLOAT2 rightStick = { 0.0f, 0.0f };
};

class InputManager {
private:
    FPSCommand m_command;
    FPSCommand m_prevCommand;
    bool m_mouseLocked = false;
    HWND m_hWnd = nullptr;

    static constexpr float STICK_DEADZONE = 0.2f;
    static constexpr float GAMEPAD_LOOK_SENSITIVITY = 3.0f;

    InputManager() = default;

public:
    static InputManager& GetInstance() {
        static InputManager instance;
        return instance;
    }

    void Initialize(HWND hWnd);
    void Update();
    void Finalize();

    const FPSCommand& GetCommand() const { return m_command; }
    static const FPSCommand& Cmd() { return GetInstance().GetCommand(); }

    // マウスロック制御（FPS用）
    void SetMouseLocked(bool locked);
    bool IsMouseLocked() const { return m_mouseLocked; }

    // デバイス直接アクセス
    bool IsKeyDown(Keyboard_Keys key) const { return Keyboard_IsKeyDown(key); }
    bool IsKeyTrigger(Keyboard_Keys key) const { return Keyboard_IsKeyDownTrigger(key); }
    void GetMouseState(Mouse_State* state) const { Mouse_GetState(state); }
    bool GetGamepadState(GamepadState& state) const { return GameController::GetState(state); }

    InputManager(const InputManager&) = delete;
    InputManager& operator=(const InputManager&) = delete;
};

} // namespace Engine
