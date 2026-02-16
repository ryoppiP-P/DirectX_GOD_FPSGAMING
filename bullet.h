#pragma once
#include "main.h"
#include "game_object.h"
#include "System/Collision/box_collider.h"
#include <DirectXMath.h>

using namespace DirectX;

class Bullet {
public:
    XMFLOAT3 position;
    XMFLOAT3 velocity;
    float lifeTime;
    bool active;
    Engine::BoxCollider collider;
    GameObject visual;
    uint32_t m_collisionId = 0;

    Bullet();
    ~Bullet();

    Bullet(const Bullet&) = delete;
    Bullet& operator=(const Bullet&) = delete;
    Bullet(Bullet&&) noexcept;
    Bullet& operator=(Bullet&&) noexcept;

    void Initialize(ID3D11ShaderResourceView* texture, const XMFLOAT3& pos, const XMFLOAT3& dir);
    void Update(float deltaTime);
    void Draw();
    void Deactivate();
};
