#pragma once

#include <DirectXMath.h>
#include <memory>

namespace Engine {
    using namespace DirectX;

    enum class ColliderType {
        BOX,
        SPHERE,
        CAPSULE,
        MESH    // 将来用
    };

    class Collider {
    public:
        virtual ~Collider() = default;

        // 型の取得
        virtual ColliderType GetType() const = 0;

        // 衝突判定（ダブルディスパッチ用）
        virtual bool Intersects(const Collider* pOther) const = 0;

        // 中心座標
        virtual XMFLOAT3 GetCenter() const = 0;

        // 境界ボックス取得（流体/空間分割用）
        virtual void GetBounds(XMFLOAT3& outMin, XMFLOAT3& outMax) const = 0;

        // トランスフォーム適用
        virtual void SetTransform(const XMFLOAT3& position, const XMFLOAT3& rotation, const XMFLOAT3& scale) = 0;

    protected:
        XMFLOAT3 m_position = { 0.0f, 0.0f, 0.0f };
        XMFLOAT3 m_rotation = { 0.0f, 0.0f, 0.0f };
        XMFLOAT3 m_scale = { 1.0f, 1.0f, 1.0f };
    };
}
