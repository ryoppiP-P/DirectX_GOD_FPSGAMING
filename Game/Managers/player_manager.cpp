/*********************************************************************
  \file    プレイヤーマネージャー [player_manager.cpp]
  
  \Author  Ryoto Kikuchi
  \data    2025
 *********************************************************************/
#include "player_manager.h"
#include "bullet_manager.h"
#include "Game/Objects/bullet.h"
#include "Game/Objects/camera.h"
#include "Game/Map/map.h"
#include "Game/Map/map_renderer.h"
#include "Engine/Graphics/primitive.h"
#include "Engine/Input/keyboard.h"
#include "Engine/Input/game_controller.h"
#include <cmath>
#include <algorithm>
#include <memory>

// ========== PlayerManager ==========

PlayerManager* PlayerManager::instance = nullptr;

PlayerManager::PlayerManager()
    : activePlayerId(1)
    , player1Initialized(false)
    , player2Initialized(false)
    , initialPlayerLocked(false) {
}

PlayerManager& PlayerManager::GetInstance() {
    if (!instance) {
        instance = new PlayerManager();
    }
    return *instance;
}

void PlayerManager::SetInitialActivePlayer(int playerId) {
    if (playerId == 1 || playerId == 2) {
        initialPlayerLocked = true;
        activePlayerId = playerId;
    }
}

void PlayerManager::Initialize(Map* map, ID3D11ShaderResourceView* texture) {
    if (initialPlayerLocked) {
        if (activePlayerId == 1) {
            if (!player1Initialized) {
                player1.Initialize(map, texture, 1, ViewMode::THIRD_PERSON);
                player1.SetPosition(XMFLOAT3(0.0f, 3.0f, 0.0f));
                player1Initialized = true;
            }
            player2Initialized = false;
        } else {
            if (!player2Initialized) {
                player2.Initialize(map, texture, 2, ViewMode::FIRST_PERSON);
                player2.SetPosition(XMFLOAT3(3.0f, 3.0f, 0.0f));
                player2Initialized = true;
            }
            player1Initialized = false;
        }
        return;
    }

    if (!player1Initialized) {
        player1.Initialize(map, texture, 1, ViewMode::THIRD_PERSON);
        player1.SetPosition(XMFLOAT3(0.0f, 3.0f, 0.0f));
        player1Initialized = true;
    }

    if (!player2Initialized) {
        player2.Initialize(map, texture, 2, ViewMode::FIRST_PERSON);
        player2.SetPosition(XMFLOAT3(3.0f, 3.0f, 0.0f));
        player2Initialized = true;
    }

    activePlayerId = 1;
}

Player* PlayerManager::GetActivePlayer() {
    if (activePlayerId == 1 && player1Initialized) return &player1;
    if (activePlayerId == 2 && player2Initialized) return &player2;
    return nullptr;
}

Player* PlayerManager::GetPlayer(int playerId) {
    if (playerId == 1 && player1Initialized) return &player1;
    if (playerId == 2 && player2Initialized) return &player2;
    return nullptr;
}

void PlayerManager::Update(float deltaTime) {
    HandleInput(deltaTime);

    if (initialPlayerLocked) {
        Player* p = GetActivePlayer();
        if (p) p->Update(deltaTime);
    } else {
        if (player1Initialized) player1.Update(deltaTime);
        if (player2Initialized) player2.Update(deltaTime);
    }

    BulletManager::GetInstance().Update(deltaTime);
}

void PlayerManager::Draw() {
    if (initialPlayerLocked) {
        Player* p = GetActivePlayer();
        if (p) p->Draw();
    } else {
        if (player1Initialized) player1.Draw();
        if (player2Initialized) player2.Draw();
    }

    BulletManager::GetInstance().Draw();
}

void PlayerManager::SetActivePlayer(int playerId) {
    if (initialPlayerLocked) return;

    if (playerId == 1 || playerId == 2) {
        Player* current = GetActivePlayer();
        CameraManager& camMgr = CameraManager::GetInstance();
        if (current) {
            current->SetCameraAngles(camMgr.GetRotation(), camMgr.GetPitch());
        }

        activePlayerId = playerId;

        Player* next = GetActivePlayer();
        if (next) {
            camMgr.SetRotation(next->GetCameraYaw());
            camMgr.SetPitch(next->GetCameraPitch());
            camMgr.UpdateCameraForPlayer(activePlayerId);
        }
    }
}

void PlayerManager::HandleInput(float deltaTime) {
    static bool was1Down = false;
    static bool was2Down = false;

    if (!initialPlayerLocked) {
        if (Keyboard_IsKeyDown(KK_D1) && !was1Down) SetActivePlayer(1);
        if (Keyboard_IsKeyDown(KK_D2) && !was2Down) SetActivePlayer(2);
    }

    was1Down = Keyboard_IsKeyDown(KK_D1);
    was2Down = Keyboard_IsKeyDown(KK_D2);

    Player* activePlayer = GetActivePlayer();
    if (!activePlayer) return;

    XMFLOAT3 moveDirection = { 0.0f, 0.0f, 0.0f };

    float yawRad = XMConvertToRadians(activePlayer->GetRotation().y);
    XMFLOAT3 forward = { sinf(yawRad), 0.0f, cosf(yawRad) };
    XMFLOAT3 right = { cosf(yawRad), 0.0f, -sinf(yawRad) };

    if (Keyboard_IsKeyDown(KK_W)) { moveDirection.x += forward.x; moveDirection.z += forward.z; }
    if (Keyboard_IsKeyDown(KK_S)) { moveDirection.x -= forward.x; moveDirection.z -= forward.z; }
    if (Keyboard_IsKeyDown(KK_A)) { moveDirection.x -= right.x; moveDirection.z -= right.z; }
    if (Keyboard_IsKeyDown(KK_D)) { moveDirection.x += right.x; moveDirection.z += right.z; }

    GamepadState padState;
    if (GameController::GetState(padState)) {
        float lx = padState.leftStickX;
        float ly = padState.leftStickY;
        const float deadzone = 0.2f;
        if (fabsf(lx) > deadzone || fabsf(ly) > deadzone) {
            moveDirection.x += -forward.x * ly + right.x * lx;
            moveDirection.z += -forward.z * ly + right.z * lx;
        }
    }

    float moveLength = sqrtf(moveDirection.x * moveDirection.x + moveDirection.z * moveDirection.z);
    if (moveLength > 0.0f) {
        moveDirection.x /= moveLength;
        moveDirection.z /= moveLength;
    }

    activePlayer->Move(moveDirection, deltaTime);

    if (Keyboard_IsKeyDown(KK_SPACE)) {
        activePlayer->Jump();
    }

    if (activePlayer->GetPlayerId() == 2 && activePlayer->IsAlive() && Keyboard_IsKeyDownTrigger(KK_ENTER)) {
        XMFLOAT3 pos = activePlayer->GetPosition();
        XMFLOAT3 dir = { sinf(yawRad), 0.0f, cosf(yawRad) };

        auto b = std::make_unique<Bullet>();
        b->Initialize(GetPolygonTexture(), pos, dir);
        BulletManager::GetInstance().Add(std::move(b));
    }
}

void InitializePlayers(Map* map, ID3D11ShaderResourceView* texture) {
    PlayerManager::GetInstance().Initialize(map, texture);
}

void UpdatePlayers() {
    constexpr float fixedDelta = 1.0f / 60.0f;
    PlayerManager::GetInstance().Update(fixedDelta);
}

void DrawPlayers() {
    PlayerManager::GetInstance().Draw();
}

GameObject* GetActivePlayerGameObject() {
    Player* activePlayer = PlayerManager::GetInstance().GetActivePlayer();
    return activePlayer ? activePlayer->GetGameObject() : nullptr;
}

Player* GetActivePlayer() {
    return PlayerManager::GetInstance().GetActivePlayer();
}
