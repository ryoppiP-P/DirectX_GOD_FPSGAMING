#include "collision_system.h"
#include "box_collider.h"
#include "sphere_collider.h"
#include <algorithm>

namespace Engine {
    CollisionSystem& CollisionSystem::GetInstance() {
        static CollisionSystem instance;
        return instance;
    }

    void CollisionSystem::RegisterCollider(Collider* pCollider) {
        if (!pCollider) return;

        auto it = std::find(m_colliders.begin(), m_colliders.end(), pCollider);
        if (it == m_colliders.end()) {
            m_colliders.push_back(pCollider);
        }
    }

    void CollisionSystem::UnregisterCollider(Collider* pCollider) {
        auto it = std::find(m_colliders.begin(), m_colliders.end(), pCollider);
        if (it != m_colliders.end()) {
            m_colliders.erase(it);
        }
    }

    void CollisionSystem::Update() {
        // 全ペアチェック（O(n^2) - 将来的に空間分割で最適化）
        for (size_t i = 0; i < m_colliders.size(); ++i) {
            for (size_t j = i + 1; j < m_colliders.size(); ++j) {
                CollisionResult result;
                if (CheckCollision(m_colliders[i], m_colliders[j], result)) {
                    if (m_callback) {
                        m_callback(result);
                    }
                }
            }
        }
    }

    void CollisionSystem::SetCollisionCallback(CollisionCallback callback) {
        m_callback = std::move(callback);
    }

    bool CollisionSystem::CheckCollision(const Collider* pA, const Collider* pB, CollisionResult& outResult) {
        if (!pA || !pB) return false;

        bool hit = pA->Intersects(pB);
        if (hit) {
            outResult.pColliderA = const_cast<Collider*>(pA);
            outResult.pColliderB = const_cast<Collider*>(pB);
            // TODO: penetration計算
        }
        return hit;
    }

    bool CollisionSystem::Raycast(const XMFLOAT3& origin, const XMFLOAT3& direction, float maxDistance, CollisionResult& outResult) {
        // 将来実装
        return false;
    }
}
