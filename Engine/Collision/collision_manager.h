/*********************************************************************
  \file    コリジョンマネージャー [collision_manager.h]
  
  \Author  Ryoto Kikuchi
  \data    2025
 *********************************************************************/
#pragma once

#include "collision_system.h"
#include "map_collision.h"
#include "collider.h"
#include <DirectXMath.h>

using namespace DirectX;

// CollisionManagerはCollisionSystemとMapCollisionへの一元的なアクセスを提供する
// シングルトンパターンで実装
class CollisionManager {
public:
    static CollisionManager& GetInstance() {
        static CollisionManager instance;
        return instance;
    }

    void Initialize(float mapCellSize = 2.0f) {
        Engine::CollisionSystem::GetInstance().Initialize();
        Engine::MapCollision::GetInstance().Initialize(mapCellSize);
    }

    void Shutdown() {
        Engine::CollisionSystem::GetInstance().Shutdown();
        Engine::MapCollision::GetInstance().Shutdown();
    }

    void Update() {
        Engine::CollisionSystem::GetInstance().Update();
    }

    // 動的オブジェクト登録
    uint32_t Register(Engine::Collider* collider, Engine::CollisionLayer layer,
                      Engine::CollisionLayer mask, void* userData) {
        return Engine::CollisionSystem::GetInstance().Register(collider, layer, mask, userData);
    }

    void Unregister(uint32_t id) {
        Engine::CollisionSystem::GetInstance().Unregister(id);
    }

    void SetEnabled(uint32_t id, bool enabled) {
        Engine::CollisionSystem::GetInstance().SetEnabled(id, enabled);
    }

    void SetCallback(Engine::CollisionCallback callback) {
        Engine::CollisionSystem::GetInstance().SetCallback(std::move(callback));
    }

    // マップコリジョン
    void RegisterMapBlock(Engine::BoxCollider* block) {
        Engine::MapCollision::GetInstance().RegisterBlock(block);
    }

    bool CheckMapCollision(Engine::BoxCollider* collider, XMFLOAT3& penetration) {
        return Engine::MapCollision::GetInstance().CheckCollision(collider, penetration);
    }

    std::vector<XMFLOAT3> CheckMapCollisionAll(Engine::BoxCollider* collider, float radius) {
        return Engine::MapCollision::GetInstance().CheckCollisionAll(collider, radius);
    }

    // 内部システムへの直接アクセス（互換性のため）
    Engine::CollisionSystem& GetCollisionSystem() {
        return Engine::CollisionSystem::GetInstance();
    }

    Engine::MapCollision& GetMapCollision() {
        return Engine::MapCollision::GetInstance();
    }

private:
    CollisionManager() = default;
    ~CollisionManager() = default;
    CollisionManager(const CollisionManager&) = delete;
    CollisionManager& operator=(const CollisionManager&) = delete;
};
