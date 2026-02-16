#pragma once

#include "collider.h"

namespace Engine {

    class SphereCollider : public Collider {
    public:
        SphereCollider();
        SphereCollider(const XMFLOAT3& center, float radius);
        ~SphereCollider() override = default;

        ColliderType GetType() const override { return ColliderType::SPHERE; }
        bool Intersects(const Collider* other) const override;
        XMFLOAT3 GetCenter() const override;
        void GetBounds(XMFLOAT3& outMin, XMFLOAT3& outMax) const override;

        void SetCenter(const XMFLOAT3& center);
        void SetRadius(float radius);

        float GetRadius() const { return m_radius; }
        float GetWorldRadius() const { return m_worldRadius; }

    private:
        void UpdateWorldRadius();

        float m_radius = 1.0f;
        float m_worldRadius = 1.0f;
    };

} // namespace Engine
