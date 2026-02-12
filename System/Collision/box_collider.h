// System/Collision/BoxCollider.h
#pragma once

#include "collider.h"

namespace Engine {
    class BoxCollider : public Collider {
    public:
        BoxCollider();
        BoxCollider(const XMFLOAT3& center, const XMFLOAT3& size);
        ~BoxCollider() override = default;

        // Collider インターフェース実装
        ColliderType GetType() const override { return ColliderType::BOX; }
        bool Intersects(const Collider* pOther) const override;
        XMFLOAT3 GetCenter() const override;
        void GetBounds(XMFLOAT3& outMin, XMFLOAT3& outMax) const override;
        void SetTransform(const XMFLOAT3& position, const XMFLOAT3& rotation, const XMFLOAT3& scale) override;

        // Box固有
        void SetSize(const XMFLOAT3& size);
        XMFLOAT3 GetSize() const { return m_size; }
        XMFLOAT3 GetMin() const { return m_worldMin; }
        XMFLOAT3 GetMax() const { return m_worldMax; }

        // 点の包含判定
        bool Contains(const XMFLOAT3& point) const;

        // ファクトリ
        static std::unique_ptr<BoxCollider> CreateFromCenterAndSize(const XMFLOAT3& center, const XMFLOAT3& size);

    private:
        void UpdateWorldBounds();

        // Box固有の衝突判定
        bool IntersectsBox(const BoxCollider* pOther) const;
        bool IntersectsSphere(const class SphereCollider* pOther) const;

        XMFLOAT3 m_size = { 1.0f, 1.0f, 1.0f };      // ローカルサイズ
        XMFLOAT3 m_worldMin = { 0.0f, 0.0f, 0.0f };  // ワールド座標の最小点
        XMFLOAT3 m_worldMax = { 0.0f, 0.0f, 0.0f };  // ワールド座標の最大点
    };
}
