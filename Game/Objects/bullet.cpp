#include "bullet.h"
#include "Engine/Core/renderer.h"
#include "Engine/Graphics/primitive.h"
#include "Engine/Collision/collision_system.h"
#include "Engine/Collision/map_collision.h"

Bullet::Bullet()
    : position(0, 0, 0)
    , velocity(0, 0, 0)
    , lifeTime(0)
    , active(false)
    , collider({ 0, 0, 0 }, { 0.2f, 0.2f, 0.2f })
    , m_collisionId(0) {
}

Bullet::~Bullet() {
    if (m_collisionId != 0) {
        Engine::CollisionSystem::GetInstance().Unregister(m_collisionId);
        m_collisionId = 0;
    }
}

Bullet::Bullet(Bullet&& other) noexcept
    : position(other.position)
    , velocity(other.velocity)
    , lifeTime(other.lifeTime)
    , active(other.active)
    , collider(std::move(other.collider))
    , visual(std::move(other.visual))
    , m_collisionId(other.m_collisionId) {
    other.m_collisionId = 0;
}

Bullet& Bullet::operator=(Bullet&& other) noexcept {
    if (this != &other) {
        if (m_collisionId != 0) {
            Engine::CollisionSystem::GetInstance().Unregister(m_collisionId);
        }

        position = other.position;
        velocity = other.velocity;
        lifeTime = other.lifeTime;
        active = other.active;
        collider = std::move(other.collider);
        visual = std::move(other.visual);
        m_collisionId = other.m_collisionId;

        other.m_collisionId = 0;
    }
    return *this;
}

void Bullet::Initialize(ID3D11ShaderResourceView* texture, const XMFLOAT3& pos, const XMFLOAT3& dir) {
    position = pos;
    velocity = { dir.x * 15.0f, dir.y * 15.0f, dir.z * 15.0f };
    lifeTime = 3.0f;
    active = true;

    visual.position = position;
    visual.scale = XMFLOAT3(0.2f, 0.2f, 0.2f);
    visual.setMesh(Box, 36, texture);
    visual.setBoxCollider(visual.scale);
    visual.markBufferForUpdate();

    collider.SetCenter(position);
    collider.SetSize({ 0.2f, 0.2f, 0.2f });

    m_collisionId = Engine::CollisionSystem::GetInstance().Register(
        &collider,
        Engine::CollisionLayer::PROJECTILE,
        Engine::CollisionLayer::PLAYER | Engine::CollisionLayer::ENEMY,
        this
    );
}

void Bullet::Update(float deltaTime) {
    if (!active) return;

    position.x += velocity.x * deltaTime;
    position.y += velocity.y * deltaTime;
    position.z += velocity.z * deltaTime;

    collider.SetCenter(position);
    visual.position = position;
    visual.markBufferForUpdate();

    // �}�b�v�Ƃ̏Փ˔���
    XMFLOAT3 pen;
    if (Engine::MapCollision::GetInstance().CheckCollision(&collider, pen)) {
        Deactivate();
        return;
    }

    lifeTime -= deltaTime;
    if (lifeTime <= 0) {
        Deactivate();
    }
}

void Bullet::Draw() {
    if (active) visual.draw();
}

void Bullet::Deactivate() {
    if (!active) return;
    active = false;

    if (m_collisionId != 0) {
        Engine::CollisionSystem::GetInstance().Unregister(m_collisionId);
        m_collisionId = 0;
    }
}
