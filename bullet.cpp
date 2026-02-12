#include "bullet.h"
#include "System/Core/renderer.h"
#include "System/Graphics/primitive.h"

Bullet::Bullet()
    : position(0, 0, 0), velocity(0, 0, 0), lifeTime(0), active(false)
{
}

void Bullet::Initialize(ID3D11ShaderResourceView* texture, const XMFLOAT3& pos, const XMFLOAT3& dir)
{
    position = pos;
    velocity = { dir.x * 15.0f, dir.y * 15.0f, dir.z * 15.0f };
    lifeTime = 3.0f;
    active = true;

    visual.position = position;
    visual.scale = XMFLOAT3(0.2f, 0.2f, 0.2f);
    visual.setMesh(Box, 36, texture);      // polygon.h �Œ�`����Ă��� Box
    visual.setBoxCollider(visual.scale);
    visual.markBufferForUpdate();

    collider = BoxCollider::fromCenterAndSize(position, visual.scale);
}

void Bullet::Update(float deltaTime)
{
    if (!active) return;

    position.x += velocity.x * deltaTime;
    position.y += velocity.y * deltaTime;
    position.z += velocity.z * deltaTime;

    collider = BoxCollider::fromCenterAndSize(position, XMFLOAT3(0.2f, 0.2f, 0.2f));
    visual.position = position;
    visual.markBufferForUpdate();

    lifeTime -= deltaTime;
    if (lifeTime <= 0) active = false;
}

void Bullet::Draw() { if (active) visual.draw(); }
void Bullet::Deactivate() { active = false; }

bool Bullet::CheckHit(const BoxCollider& target)
{
    if (!active) return false;
    return collider.intersects(target);
}
