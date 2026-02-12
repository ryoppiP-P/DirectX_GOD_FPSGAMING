// player.h
#pragma once
#include "main.h"
#include "game_object.h"
#include <DirectXMath.h>

using namespace DirectX;

class Map;

enum class ViewMode {
    THIRD_PERSON,  // TPS
    FIRST_PERSON   // FPS
};

class Player {
private:
    XMFLOAT3 position;
    XMFLOAT3 velocity;
    XMFLOAT3 rotation;
    XMFLOAT3 scale;

    BoxCollider collider;
    GameObject visualObject;

    // *** 追加 ***
    // HP追加
    int hp;
    bool isAlive;
    // *** 追加 終了 ***

    // プレイヤーのサイズ
    static constexpr float WIDTH = 0.8f;
    static constexpr float HEIGHT = 1.8f;
    static constexpr float DEPTH = 0.8f;

    // 物理パラメータ
    static constexpr float GRAVITY = -9.8f;
    static constexpr float JUMP_POWER = 5.0f;
    static constexpr float MOVE_SPEED = 5.0f;
    static constexpr float FRICTION = 0.85f;

    bool isGrounded;
    Map* mapRef;
    int playerId;
    ViewMode viewMode;
    bool colliderDirty; // コライダー更新が必要かどうかのフラグ

    // カメラ状態をプレイヤー単位で保持（プレイヤー切替時に復元するため）
    float cameraYaw;
    float cameraPitch;

    void UpdateCollider();
    void CheckMapCollision();

public:
    Player();

    void Initialize(Map* map, ID3D11ShaderResourceView* texture, int id = 0, ViewMode mode = ViewMode::THIRD_PERSON);
    void Update(float deltaTime);
    void Draw();

    void Move(const XMFLOAT3& direction, float deltaTime);
    void Jump();

    XMFLOAT3 GetPosition() const { return position; }
    XMFLOAT3 GetRotation() const { return rotation; }
    BoxCollider GetCollider() const { return collider; }
    bool IsGrounded() const { return isGrounded; }
    int GetPlayerId() const { return playerId; }
    ViewMode GetViewMode() const { return viewMode; }

    void SetPosition(const XMFLOAT3& pos);
    void SetRotation(const XMFLOAT3& rot) { rotation = rot; }
    void SetViewMode(ViewMode mode) { viewMode = mode; }

    // カメラ角度のアクセサ（本体は player.cpp に実装）
    void SetCameraAngles(float yaw, float pitch);
    float GetCameraYaw() const;
    float GetCameraPitch() const;

    // 衝突処理を NPC マネージャから呼べるように public にする
    bool CheckCollisionWithBox(const BoxCollider& box, XMFLOAT3& penetration);
    void ResolveCollision(const XMFLOAT3& penetration);

    // NPC マネージャ用ラッパー（安全に衝突判定と反映を行う）
    bool ComputePenetrationWithBox(const BoxCollider& box, XMFLOAT3& penetration);
    void ApplyPenetration(const XMFLOAT3& penetration);

    // Networking / external accessors for the underlying GameObject
    GameObject* GetGameObject();
    
    // ネットワーク用：位置強制上書き
    void ForceSetPosition(const XMFLOAT3& pos);
    void ForceSetRotation(const XMFLOAT3& rot);

    // *** 追加 ***
    // HP関連
    int GetHP() const { return hp; }
    bool IsAlive() const { return isAlive; } // getter追加
    void TakeDamage(int dmg) {
        hp -= dmg;
        if (hp <= 0) {
            hp = 0;
            isAlive = false;
        }
    }
};

// プレイヤー管理クラス
class PlayerManager {
private:
    static PlayerManager* instance;
    Player player1;
    Player player2;
    int activePlayerId;
    bool player1Initialized;
    bool player2Initialized;
    bool initialPlayerLocked; // when true, initial active player cannot be switched and other player is not used

    PlayerManager(); // コンストラクターを宣言

public:
    static PlayerManager& GetInstance();
    
    void Initialize(Map* map, ID3D11ShaderResourceView* texture);
    // Lock which player to use for the entire run. Call before Initialize.
    void SetInitialActivePlayer(int playerId);
    void Update(float deltaTime);
    void Draw();
    
    void SetActivePlayer(int playerId);
    int GetActivePlayerId() const { return activePlayerId; }
    
    Player* GetActivePlayer();
    Player* GetPlayer(int playerId);
    
    void HandleInput(float deltaTime);
};

// グローバル関数（後方互換性のため）
void InitializePlayers(Map* map, ID3D11ShaderResourceView* texture);
void UpdatePlayers();
void DrawPlayers();
GameObject* GetActivePlayerGameObject();
Player* GetActivePlayer();