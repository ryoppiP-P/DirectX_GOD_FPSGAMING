#pragma once

#include <DirectXMath.h>

namespace Engine {
    using namespace DirectX;

    enum class ColliderType {
        BOX,
        SPHERE
    };

    class Collider {
    public:
        virtual ~Collider() = default;

        virtual ColliderType GetType() const = 0;
        virtual bool Intersects(const Collider* other) const = 0;
        virtual XMFLOAT3 GetCenter() const = 0;
        virtual void GetBounds(XMFLOAT3& outMin, XMFLOAT3& outMax) const = 0;

    protected:
        XMFLOAT3 m_position = { 0.0f, 0.0f, 0.0f };
        XMFLOAT3 m_rotation = { 0.0f, 0.0f, 0.0f };
        XMFLOAT3 m_scale = { 1.0f, 1.0f, 1.0f };
    };

} // namespace Engine
