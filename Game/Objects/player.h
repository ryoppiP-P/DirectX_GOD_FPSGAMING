#pragma once
#include "main.h"
#include "Game/Objects/game_object.h"
#include "Engine/Collision/box_collider.h"
#include <DirectXMath.h>

using namespace DirectX;

namespace Game {

class Map;

enum class ViewMode {
    THIRD_PERSON,
    FIRST_PERSON
};

class Player {
private:
    XMFLOAT3 position;
    XMFLOAT3 velocity;
    XMFLOAT3 rotation;
    XMFLOAT3 scale;

    Engine::BoxCollider collider;
    GameObject visualObject;
    uint32_t m_collisionId = 0;

    int hp;
    bool isAlive;

    static constexpr float WIDTH = 0.8f;
    static constexpr float HEIGHT = 1.8f;
    static constexpr float DEPTH = 0.8f;

    static constexpr float GRAVITY = -9.8f;
    static constexpr float JUMP_POWER = 5.0f;
    static constexpr float MOVE_SPEED = 5.0f;
    static constexpr float FRICTION = 0.85f;

    bool isGrounded;
    Map* mapRef;
    int playerId;
    ViewMode viewMode;

    float cameraYaw;
    float cameraPitch;

    void UpdateCollider();

public:
    Player();
    ~Player();

    void Initialize(Map* map, ID3D11ShaderResourceView* texture, int id = 0, ViewMode mode = ViewMode::THIRD_PERSON);
    void Update(float deltaTime);
    void Draw();

    void Move(const XMFLOAT3& direction, float deltaTime);
    void Jump();

    XMFLOAT3 GetPosition() const { return position; }
    XMFLOAT3 GetRotation() const { return rotation; }
    const Engine::BoxCollider& GetCollider() const { return collider; }
    Engine::BoxCollider* GetColliderPtr() { return &collider; }
    bool IsGrounded() const { return isGrounded; }
    int GetPlayerId() const { return playerId; }
    ViewMode GetViewMode() const { return viewMode; }
    uint32_t GetCollisionId() const { return m_collisionId; }

    void SetPosition(const XMFLOAT3& pos);
    void SetRotation(const XMFLOAT3& rot) { rotation = rot; }
    void SetViewMode(ViewMode mode) { viewMode = mode; }
    void SetGrounded(bool grounded) { isGrounded = grounded; }

    void SetCameraAngles(float yaw, float pitch);
    float GetCameraYaw() const;
    float GetCameraPitch() const;

    void ApplyPenetration(const XMFLOAT3& penetration);

    GameObject* GetGameObject();

    void ForceSetPosition(const XMFLOAT3& pos);
    void ForceSetRotation(const XMFLOAT3& rot);

    int GetHP() const { return hp; }
    bool IsAlive() const { return isAlive; }
    void TakeDamage(int dmg);
    void Respawn(const XMFLOAT3& spawnPoint);
};

} // namespace Game
