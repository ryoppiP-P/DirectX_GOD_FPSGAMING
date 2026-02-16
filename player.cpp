#include "player.h"
#include "map.h"
#include "map_renderer.h"
#include "System/Graphics/primitive.h"
#include "System/Collision/collision_system.h"
#include "System/Collision/map_collision.h"
#include "NetWork/network_manager.h"
#include "keyboard.h"
#include "camera.h"
#include "game_controller.h"
#include "bullet.h"
#include <cmath>
#include <algorithm>
#include <memory>
#include <vector>

static std::vector<std::unique_ptr<Bullet>> g_bullets;

Player::Player()
    : position(0.0f, 3.0f, 0.0f)
    , velocity(0.0f, 0.0f, 0.0f)
    , rotation(0.0f, 0.0f, 0.0f)
    , scale(WIDTH, HEIGHT, DEPTH)
    , m_collisionId(0)
    , hp(100)
    , isAlive(true)
    , isGrounded(false)
    , mapRef(nullptr)
    , playerId(0)
    , viewMode(ViewMode::THIRD_PERSON)
    , cameraYaw(0.0f)
    , cameraPitch(0.0f) {
    collider.SetCenter(position);
    collider.SetSize(scale);
}

Player::~Player() {
    if (m_collisionId != 0) {
        Engine::CollisionSystem::GetInstance().Unregister(m_collisionId);
        m_collisionId = 0;
    }
}

void Player::Initialize(Map* map, ID3D11ShaderResourceView* texture, int id, ViewMode mode) {
    mapRef = map;
    playerId = id;
    viewMode = mode;

    if (playerId == 2) {
        position = XMFLOAT3(3.0f, 3.0f, 0.0f);
    }

    visualObject.position = position;
    visualObject.scale = scale;
    visualObject.rotation = rotation;
    visualObject.setMesh(Box, 36, texture);
    visualObject.setBoxCollider(scale);
    visualObject.markBufferForUpdate();

    collider.SetCenter(position);
    collider.SetSize(scale);

    m_collisionId = Engine::CollisionSystem::GetInstance().Register(
        &collider,
        Engine::CollisionLayer::PLAYER,
        Engine::CollisionLayer::PROJECTILE | Engine::CollisionLayer::ENEMY,
        this
    );
}

void Player::SetPosition(const XMFLOAT3& pos) {
    position = pos;
    UpdateCollider();
}

void Player::UpdateCollider() {
    collider.SetCenter(position);
    collider.SetSize(scale);

    visualObject.position = position;
    visualObject.scale = scale;
    visualObject.setBoxCollider(scale);
    visualObject.markBufferForUpdate();
}

void Player::Update(float deltaTime) {
    if (!isAlive) return;

    if (!isGrounded) {
        velocity.y += GRAVITY * deltaTime;
    }

    position.x += velocity.x * deltaTime;
    position.y += velocity.y * deltaTime;
    position.z += velocity.z * deltaTime;

    UpdateCollider();

    // マップとの衝突判定（グリッドベース）
    isGrounded = false;
    auto penetrations = Engine::MapCollision::GetInstance().CheckCollisionAll(&collider, 3.0f);
    for (const auto& pen : penetrations) {
        ApplyPenetration(pen);
    }

    velocity.x *= FRICTION;
    velocity.z *= FRICTION;

    if (fabsf(velocity.x) < 0.01f) velocity.x = 0.0f;
    if (fabsf(velocity.z) < 0.01f) velocity.z = 0.0f;

    visualObject.position = position;
    visualObject.rotation = rotation;
    visualObject.markBufferForUpdate();
}

void Player::Move(const XMFLOAT3& direction, float deltaTime) {
    velocity.x = direction.x * MOVE_SPEED;
    velocity.z = direction.z * MOVE_SPEED;
}

void Player::Jump() {
    if (isGrounded) {
        velocity.y = JUMP_POWER;
        isGrounded = false;
    }
}

void Player::ApplyPenetration(const XMFLOAT3& penetration) {
    position.x += penetration.x;
    position.y += penetration.y;
    position.z += penetration.z;

    if (penetration.x != 0.0f) velocity.x = 0.0f;
    if (penetration.y != 0.0f) {
        velocity.y = 0.0f;
        if (penetration.y > 0.0f) {
            isGrounded = true;
        }
    }
    if (penetration.z != 0.0f) velocity.z = 0.0f;

    UpdateCollider();
}

void Player::Draw() {
    if (!isAlive) return;
    visualObject.draw();
}

GameObject* Player::GetGameObject() {
    return &visualObject;
}

void Player::ForceSetPosition(const XMFLOAT3& pos) {
    position = pos;
    if (mapRef) {
        float groundY = mapRef->GetGroundHeight(position.x, position.z);
        if (position.y < groundY + scale.y * 0.5f) {
            position.y = groundY + scale.y * 0.5f;
        }
    }
    UpdateCollider();
}

void Player::ForceSetRotation(const XMFLOAT3& rot) {
    rotation = rot;
    visualObject.rotation = rotation;
    visualObject.markBufferForUpdate();
}

void Player::SetCameraAngles(float yaw, float pitch) {
    cameraYaw = yaw;
    cameraPitch = pitch;
}

float Player::GetCameraYaw() const { return cameraYaw; }
float Player::GetCameraPitch() const { return cameraPitch; }

void Player::TakeDamage(int dmg) {
    hp -= dmg;
    if (hp <= 0) {
        hp = 0;
        isAlive = false;
    }
}

void Player::Respawn(const XMFLOAT3& spawnPoint) {
    position = spawnPoint;
    hp = 100;
    isAlive = true;
    velocity = { 0.0f, 0.0f, 0.0f };
    isGrounded = false;
    UpdateCollider();
}

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

    for (auto& b : g_bullets) {
        b->Update(deltaTime);
    }

    g_bullets.erase(
        std::remove_if(g_bullets.begin(), g_bullets.end(),
            [](const std::unique_ptr<Bullet>& b) { return !b->active; }),
        g_bullets.end());
}

void PlayerManager::Draw() {
    if (initialPlayerLocked) {
        Player* p = GetActivePlayer();
        if (p) p->Draw();
    } else {
        if (player1Initialized) player1.Draw();
        if (player2Initialized) player2.Draw();
    }

    for (auto& b : g_bullets) {
        b->Draw();
    }
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
        g_bullets.push_back(std::move(b));
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
