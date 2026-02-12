// player.cpp
#include "player.h"
#include "map.h"
#include "map_renderer.h"
#include "polygon.h"
#include "NetWork/network_manager.h"
#include "keyboard.h"
#include "camera.h" // カメラ同期のために追加
#include <cmath>
#include <algorithm>
#include "game_controller.h" // コントローラ入力

#include "bullet.h"
#include <memory>
#include <vector>
static std::vector<std::unique_ptr<Bullet>> g_bullets;
extern ID3D11ShaderResourceView* GetPolygonTexture(); // 弾用テクスチャ取得関数


// minの代わりに使うヘルパー関数
static inline float Min(float a, float b) {
    return (a < b) ? a : b;
}

static inline float Max(float a, float b) {
    return (a > b) ? a : b;
}

Player::Player()
    : position(0.0f, 3.0f, 0.0f)
    , velocity(0.0f, 0.0f, 0.0f)
    , rotation(0.0f, 0.0f, 0.0f)
    , scale(WIDTH, HEIGHT, DEPTH)
    , isGrounded(false)
    , mapRef(nullptr)
    , playerId(0)
    , viewMode(ViewMode::THIRD_PERSON)
    , cameraYaw(0.0f)
    , cameraPitch(0.0f)
    , hp(100)
    , isAlive(true)
{
    UpdateCollider();
}

void Player::Initialize(Map* map, ID3D11ShaderResourceView* texture, int id, ViewMode mode) {
    mapRef = map;
    playerId = id;
    viewMode = mode;

    // プレイヤー2は少し離れた位置に配置
    if (playerId == 2) {
        position = XMFLOAT3(3.0f, 3.0f, 0.0f);
    }

    // ビジュアルオブジェクトの設定
    visualObject.position = position;
    visualObject.scale = scale;
    visualObject.rotation = rotation;
    visualObject.setMesh(Box, 36, texture);
    visualObject.setBoxCollider(scale);
    visualObject.markBufferForUpdate();
}

void Player::SetPosition(const XMFLOAT3& pos) {
    position = pos;
    UpdateCollider();
}

void Player::UpdateCollider() {
    collider = BoxCollider::fromCenterAndSize(position, scale);
    // GameObjectのBoxColliderも追従させる
    visualObject.position = position;
    visualObject.scale = scale;
    visualObject.setBoxCollider(scale);
    visualObject.markBufferForUpdate();
}

void Player::Update(float deltaTime) {
   
    // *** 追加 ***
    if (!isAlive) return;
    // *** 追加ここまで ***
    // 前回の位置を記録
    XMFLOAT3 previousPosition = position;

    // 重力適用
    if (!isGrounded) {
        velocity.y += GRAVITY * deltaTime;
    }

    // 速度を位置に適用
    position.x += velocity.x * deltaTime;
    position.y += velocity.y * deltaTime;
    position.z += velocity.z * deltaTime;

    // 位置が変わった場合のみコライダー更新
    bool positionChanged = (fabsf(position.x - previousPosition.x) > 0.001f ||
        fabsf(position.y - previousPosition.y) > 0.001f ||
        fabsf(position.z - previousPosition.z) > 0.001f);

    if (positionChanged) {
        UpdateCollider();
    }

    // マップとの衝突判定
    isGrounded = false;
    CheckMapCollision();

    // 摩擦
    velocity.x *= FRICTION;
    velocity.z *= FRICTION;

    // 極小の速度を0にする
    if (fabsf(velocity.x) < 0.01f) velocity.x = 0.0f;
    if (fabsf(velocity.z) < 0.01f) velocity.z = 0.0f;

    // ビジュアルオブジェクト更新（位置が変わった場合のみ）
    if (positionChanged) {
        visualObject.position = position;
        visualObject.rotation = rotation;
        visualObject.markBufferForUpdate();
    }
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

void Player::CheckMapCollision() {
    if (!mapRef) return;
    isGrounded = false;

    // プレイヤーの近くのブロックのみチェック（粗い最適化）
    const auto& blocks = mapRef->GetBlockObjects();
    const float checkRadius = 5.0f; // プレイヤーから5ユニット以内のブロックのみチェック

    for (const auto& blockPtr : blocks) {
        const auto& block = *blockPtr;

        // 距離チェックによる早期カット
        XMFLOAT3 blockPos = block.getPosition();
        float distanceSquared = (position.x - blockPos.x) * (position.x - blockPos.x) +
            (position.y - blockPos.y) * (position.y - blockPos.y) +
            (position.z - blockPos.z) * (position.z - blockPos.z);

        if (distanceSquared > checkRadius * checkRadius) {
            continue; // 遠すぎるブロックはスキップ
        }

        if (block.colliderType == ColliderType::Box && block.boxCollider) {
            XMFLOAT3 penetration;
            if (visualObject.checkCollision(block)) {
                if (CheckCollisionWithBox(*block.boxCollider, penetration)) {
                    ResolveCollision(penetration);
                }
            }
        }
    }
}

bool Player::CheckCollisionWithBox(const BoxCollider& box, XMFLOAT3& penetration) {
    if (!collider.intersects(box)) {
        return false;
    }

    // 各軸での重なり量を計算（std::minの代わりにMin関数を使用）
    float overlapX = Min(collider.max.x - box.min.x, box.max.x - collider.min.x);
    float overlapY = Min(collider.max.y - box.min.y, box.max.y - collider.min.y);
    float overlapZ = Min(collider.max.z - box.min.z, box.max.z - collider.min.z);

    // 最小の重なり軸を見つける
    if (overlapX < overlapY && overlapX < overlapZ) {
        // X軸で押し戻し
        float boxCenterX = box.min.x + (box.max.x - box.min.x) * 0.5f;
        penetration.x = (position.x < boxCenterX) ? -overlapX : overlapX;
        penetration.y = 0.0f;
        penetration.z = 0.0f;
    } else if (overlapY < overlapZ) {
        // Y軸で押し戻し
        float boxCenterY = box.min.y + (box.max.y - box.min.y) * 0.5f;
        penetration.x = 0.0f;
        penetration.y = (position.y < boxCenterY) ? -overlapY : overlapY;
        penetration.z = 0.0f;
    } else {
        // Z軸で押し戻し
        float boxCenterZ = box.min.z + (box.max.z - box.min.z) * 0.5f;
        penetration.x = 0.0f;
        penetration.y = 0.0f;
        penetration.z = (position.z < boxCenterZ) ? -overlapZ : overlapZ;
    }

    return true;
}

void Player::ResolveCollision(const XMFLOAT3& penetration) {
    // 位置を補正
    position.x += penetration.x;
    position.y += penetration.y;
    position.z += penetration.z;

    // 速度を補正
    if (penetration.x != 0.0f) {
        velocity.x = 0.0f;
    }
    if (penetration.y != 0.0f) {
        velocity.y = 0.0f;
        // 上から押し戻された = 地面に接地
        if (penetration.y > 0.0f) {
            isGrounded = true;
        }
    }
    if (penetration.z != 0.0f) {
        velocity.z = 0.0f;
    }

    // コライダー更新
    UpdateCollider();
}

void Player::Draw() {
    // *** 追加 ***
    if (!isAlive) return;
    // *** 追加ここまで ***
    visualObject.draw();
}

GameObject* Player::GetGameObject() {
    return &visualObject;
}

// ネットワーク用：プレイヤー強制移動（地面調整付き）
void Player::ForceSetPosition(const XMFLOAT3& pos) {
    position = pos;

    // 地面の高さを確認して調整
    if (mapRef) {
        float groundY = mapRef->GetGroundHeight(position.x, position.z);
        // Y座標が地面より低い場合は地面に調整
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

// カメラ角度関連の設定
void Player::SetCameraAngles(float yaw, float pitch) {
    cameraYaw = yaw;
    cameraPitch = pitch;
}

float Player::GetCameraYaw() const {
    return cameraYaw;
}

float Player::GetCameraPitch() const {
    return cameraPitch;
}

// NPC 用：衝突量を計算する（外部から安全に呼べる）
bool Player::ComputePenetrationWithBox(const BoxCollider& box, XMFLOAT3& penetration) {
    //ここではプレイヤーのコライダーと指定 box の重なりを判定し penetration を返す
    return CheckCollisionWithBox(box, penetration);
}

void Player::ApplyPenetration(const XMFLOAT3& penetration) {
    //既存のResolveCollision を利用して位置と速度を更新
    ResolveCollision(penetration);
}

// PlayerManager実装
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
    if (playerId ==1 || playerId ==2) {
        initialPlayerLocked = true;
        activePlayerId = playerId;
    }
}

void PlayerManager::Initialize(Map* map, ID3D11ShaderResourceView* texture) {
    if (initialPlayerLocked) {
        // Initialize only the chosen player, do not create the other
        if (activePlayerId ==1) {
            if (!player1Initialized) {
                player1.Initialize(map, texture,1, ViewMode::THIRD_PERSON);
                player1.SetPosition(XMFLOAT3(0.0f,3.0f,0.0f));
                player1Initialized = true;
            }
            player2Initialized = false;
        } else { // activePlayerId ==2
            if (!player2Initialized) {
                player2.Initialize(map, texture,2, ViewMode::FIRST_PERSON);
                player2.SetPosition(XMFLOAT3(3.0f,3.0f,0.0f));
                player2Initialized = true;
            }
            player1Initialized = false;
        }
        // activePlayerId already set via SetInitialActivePlayer
        return;
    }

    if (!player1Initialized) {
        player1.Initialize(map, texture,1, ViewMode::THIRD_PERSON);
        player1.SetPosition(XMFLOAT3(0.0f,3.0f,0.0f));
        player1Initialized = true;
    }

    if (!player2Initialized) {
        player2.Initialize(map, texture,2, ViewMode::FIRST_PERSON);
        player2.SetPosition(XMFLOAT3(3.0f,3.0f,0.0f));
        player2Initialized = true;
    }

    activePlayerId =1; // デフォルトでプレイヤー1
}

void PlayerManager::Update(float deltaTime) {
    // 入力処理
    HandleInput(deltaTime);

    // If initial player locked, update only active player; otherwise update both
    if (initialPlayerLocked) {
        Player* p = GetActivePlayer();
        if (p) p->Update(deltaTime);
        return;
    }

    // 両方のプレイヤーを物理演算で更新（重力、衝突判定など）
    if (player1Initialized) {
        player1.Update(deltaTime);
    }
    if (player2Initialized) {
        player2.Update(deltaTime);
    }

    // *** 追加 ***
    // --- 弾の更新 ---
    for (auto& b : g_bullets) {
        b->Update(deltaTime);

        // --- 弾がPlayer1に当たった場合 ---
        Player* player1Ptr = GetPlayer(1);
        if (player1Ptr && player1Ptr->IsAlive() && b->CheckHit(player1Ptr->GetCollider())) {
            b->Deactivate();
            player1Ptr->TakeDamage(50); // ダメージ量調整可
        }
    }

    // 無効化された弾を削除
    g_bullets.erase(
        std::remove_if(g_bullets.begin(), g_bullets.end(),
            [](const std::unique_ptr<Bullet>& b) { return !b->active; }),
        g_bullets.end());

    // *** 追加ここまで ***
}

void PlayerManager::Draw() {
    if (initialPlayerLocked) {
        Player* p = GetActivePlayer();
        if (p) p->Draw();
        return;
    }

    // 両方のプレイヤーを描画（アクティブでない方も見えるように）
    if (player1Initialized) {
        player1.Draw();
    }
    if (player2Initialized) {
        player2.Draw();
    }

    // *** 追加 ***
    // 弾の描画
    for (auto& b : g_bullets) {
        b->Draw();
    }
    // *** 追加ここまで ***
}

void PlayerManager::SetActivePlayer(int playerId) {
    if (initialPlayerLocked) return; // switching disabled when locked

    if (playerId ==1 || playerId ==2) {
        // 現在のアクティブプレイヤーのカメラ角度を保存
        Player* current = GetActivePlayer();
        CameraManager& camMgr = CameraManager::GetInstance();
        if (current) {
            current->SetCameraAngles(camMgr.GetRotation(), camMgr.GetPitch());
        }

        // アクティブプレイヤーを切り替え
        activePlayerId = playerId;

        // 切り替え先プレイヤーのカメラ角度を復元
        Player* next = GetActivePlayer();
        if (next) {
            camMgr.SetRotation(next->GetCameraYaw());
            camMgr.SetPitch(next->GetCameraPitch());
            // カメラ位置と注視点を即座に更新
            camMgr.UpdateCameraForPlayer(activePlayerId);
        }
    }
}

void PlayerManager::HandleInput(float deltaTime) {
    // If locked, ignore switch keys entirely
    static bool was1Down = false;
    static bool was2Down = false;

    if (!initialPlayerLocked) {
        if (Keyboard_IsKeyDown(KK_D1) && !was1Down) {
            SetActivePlayer(1);
        }
        if (Keyboard_IsKeyDown(KK_D2) && !was2Down) {
            SetActivePlayer(2);
        }
    }

    was1Down = Keyboard_IsKeyDown(KK_D1);
    was2Down = Keyboard_IsKeyDown(KK_D2);

    // アクティブプレイヤーのみが入力を受け付ける
    Player* activePlayer = GetActivePlayer();
    if (!activePlayer) return;

    XMFLOAT3 moveDirection = {0.0f,0.0f,0.0f };

    // カメラの向きを考慮した移動計算
    float yawRad = XMConvertToRadians(activePlayer->GetRotation().y);
    XMFLOAT3 forward = { sinf(yawRad),0.0f, cosf(yawRad) };
    XMFLOAT3 right = { cosf(yawRad),0.0f, -sinf(yawRad) };

    if (Keyboard_IsKeyDown(KK_W)) {
        moveDirection.x += forward.x;
        moveDirection.z += forward.z;
    }
    if (Keyboard_IsKeyDown(KK_S)) {
        moveDirection.x -= forward.x;
        moveDirection.z -= forward.z;
    }
    if (Keyboard_IsKeyDown(KK_A)) {
        moveDirection.x -= right.x;
        moveDirection.z -= right.z;
    }
    if (Keyboard_IsKeyDown(KK_D)) {
        moveDirection.x += right.x;
        moveDirection.z += right.z;
    }

    // コントローラの左スティックを統合
    GamepadState padState;
    if (GameController::GetState(padState)) {
        // 左スティックの値を取得
        float lx = padState.leftStickX; // 左が-1、右が+1 の想定
        float ly = padState.leftStickY; // 前が+1、後が-1 の想定
        const float deadzone =0.2f;
        if (fabsf(lx) > deadzone || fabsf(ly) > deadzone) {
            // WASD と同じ方向になるようにマッピング
            // W/S -> forward * ly, A/D -> right * lx
            moveDirection.x += -forward.x * ly + right.x * lx;
            moveDirection.z += -forward.z * ly + right.z * lx;
        }
    }

    // 移動ベクトルを正規化
    float moveLength = sqrtf(moveDirection.x * moveDirection.x + moveDirection.z * moveDirection.z);
    if (moveLength >0.0f) {
        moveDirection.x /= moveLength;
        moveDirection.z /= moveLength;
    }

    // アクティブプレイヤーのみが移動入力を受け付ける
    activePlayer->Move(moveDirection, deltaTime);

    // ジャンプもアクティブプレイヤーのみ
    if (Keyboard_IsKeyDown(KK_SPACE)) {
        activePlayer->Jump();
    }

    // *** 追加 ***
    // 弾発射（2P専用）
    if (activePlayer->GetPlayerId() == 2 && activePlayer->IsAlive() && Keyboard_IsKeyDownTrigger(KK_ENTER)) {
        XMFLOAT3 pos = activePlayer->GetPosition();
        float yawRad = XMConvertToRadians(activePlayer->GetRotation().y);
        XMFLOAT3 dir = { sinf(yawRad), 0.0f, cosf(yawRad) };

        auto b = std::make_unique<Bullet>();
        b->Initialize(GetPolygonTexture(), pos, dir);
        g_bullets.push_back(std::move(b));
    }
    // *** 追加ここまで ***
}

// グローバル関数（後方互換性）
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