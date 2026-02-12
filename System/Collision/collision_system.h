#pragma once

#include "collider.h"
#include <vector>
#include <functional>
#include <memory>

namespace Engine {
    // 衝突結果
    struct CollisionResult {
        Collider* pColliderA = nullptr;
        Collider* pColliderB = nullptr;
        XMFLOAT3 penetration = { 0.0f, 0.0f, 0.0f };
        XMFLOAT3 contactPoint = { 0.0f, 0.0f, 0.0f };
    };

    // 衝突コールバック
    using CollisionCallback = std::function<void(const CollisionResult&)>;

    class CollisionSystem {
    public:
        static CollisionSystem& GetInstance();

        // コライダー登録/解除
        void RegisterCollider(Collider* pCollider);
        void UnregisterCollider(Collider* pCollider);

        // 全衝突判定実行
        void Update();

        // コールバック登録
        void SetCollisionCallback(CollisionCallback callback);

        // 特定のコライダー同士の判定
        bool CheckCollision(const Collider* pA, const Collider* pB, CollisionResult& outResult);

        // レイキャスト（将来用）
        bool Raycast(const XMFLOAT3& origin, const XMFLOAT3& direction, float maxDistance, CollisionResult& outResult);

    private:
        CollisionSystem() = default;
        ~CollisionSystem() = default;
        CollisionSystem(const CollisionSystem&) = delete;
        CollisionSystem& operator=(const CollisionSystem&) = delete;

        std::vector<Collider*> m_colliders;
        CollisionCallback m_callback;
    };
}
