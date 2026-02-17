#pragma once

#include <DirectXMath.h>

namespace Engine {
    using namespace DirectX;

    // コライダーの形状タイプ
    enum class ColliderType {
        BOX,
        SPHERE
    };

    // コライダーの用途タイプ
    enum class ColliderPurpose {
        BODY,
        ATTACK,
        TRIGGER
    };

    class Collider {
    public:
        virtual ~Collider() = default;

        virtual ColliderType GetType() const = 0;
        virtual bool Intersects(const Collider* other) const = 0;
        virtual XMFLOAT3 GetCenter() const = 0;
        virtual void GetBounds(XMFLOAT3& outMin, XMFLOAT3& outMax) const = 0;

        ColliderPurpose GetPurpose() const { return m_purpose; }
        void SetPurpose(ColliderPurpose purpose) { m_purpose = purpose; }

    protected:
        XMFLOAT3 m_position = { 0.0f, 0.0f, 0.0f };
        XMFLOAT3 m_rotation = { 0.0f, 0.0f, 0.0f };
        XMFLOAT3 m_scale = { 1.0f, 1.0f, 1.0f };
        ColliderPurpose m_purpose = ColliderPurpose::BODY;
    };

} // namespace Engine
