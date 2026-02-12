#pragma once
#pragma once
#include "main.h"
#include "game_object.h"
#include <DirectXMath.h>

using namespace DirectX;

class Bullet {
public:
    XMFLOAT3 position;
    XMFLOAT3 velocity;
    float lifeTime;
    bool active;
    BoxCollider collider;
    GameObject visual;

    Bullet();
    Bullet(const Bullet&) = delete;            // �R�s�[�֎~
    Bullet& operator=(const Bullet&) = delete; // �R�s�[�֎~
    Bullet(Bullet&&) = default;                // ���[�u��OK
    Bullet& operator=(Bullet&&) = default;

    void Initialize(ID3D11ShaderResourceView* texture, const XMFLOAT3& pos, const XMFLOAT3& dir);
    void Update(float deltaTime);
    void Draw();
    void Deactivate();
    bool CheckHit(const BoxCollider& target);
};
