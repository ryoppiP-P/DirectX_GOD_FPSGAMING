#pragma once

#include "collider.h"

namespace Engine {
    class SphereCollider : public Collider {
    public:
        SphereCollider();
        SphereCollider(const XMFLOAT3& center, float radius);
        ~SphereCollider() override = default;

        // Collider インターフェース実装
        ColliderType GetType() const override { return ColliderType::SPHERE; }
        bool Intersects(const Collider* pOther) const override;
        XMFLOAT3 GetCenter() const override;
        void GetBounds(XMFLOAT3& outMin, XMFLOAT3& outMax) const override;
        void SetTransform(const XMFLOAT3& position, const XMFLOAT3& rotation, const XMFLOAT3& scale) override;

        // Sphere固有
        void SetRadius(float radius);
        float GetRadius() const { return m_radius; }
        float GetWorldRadius() const { return m_worldRadius; }

    private:
        void UpdateWorldRadius();

        float m_radius = 1.0f;
        float m_worldRadius = 1.0f;
    };
}