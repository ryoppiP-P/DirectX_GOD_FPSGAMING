#include "player.h"
#include "Game/Map/map.h"
#include "Engine/Graphics/primitive.h"
#include "Engine/Collision/collision_system.h"
#include "Engine/Collision/map_collision.h"
#include <cmath>
#include <algorithm>

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

    // �}�b�v�Ƃ̏Փ˔���i�O���b�h�x�[�X�j
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
