#pragma once

#include "collider.h"
#include <memory>

namespace Engine {

    class SphereCollider;

    class BoxCollider : public Collider {
    public:
        BoxCollider();
        BoxCollider(const XMFLOAT3& center, const XMFLOAT3& size);
        ~BoxCollider() override = default;

        // コピー・ムーブ
        BoxCollider(const BoxCollider& other);
        BoxCollider& operator=(const BoxCollider& other);
        BoxCollider(BoxCollider&& other) noexcept = default;
        BoxCollider& operator=(BoxCollider&& other) noexcept = default;

        // Collider実装
        ColliderType GetType() const override { return ColliderType::BOX; }
        bool Intersects(const Collider* other) const override;
        XMFLOAT3 GetCenter() const override;
        void GetBounds(XMFLOAT3& outMin, XMFLOAT3& outMax) const override;

        // 設定
        void SetCenter(const XMFLOAT3& center);
        void SetSize(const XMFLOAT3& size);
        void SetTransform(const XMFLOAT3& position, const XMFLOAT3& rotation, const XMFLOAT3& scale);

        // 取得
        XMFLOAT3 GetSize() const { return m_size; }
        XMFLOAT3 GetMin() const { return m_worldMin; }
        XMFLOAT3 GetMax() const { return m_worldMax; }

        // 判定
        bool Contains(const XMFLOAT3& point) const;
        bool ComputePenetration(const BoxCollider* other, XMFLOAT3& outPenetration) const;

        // ファクトリ
        static std::unique_ptr<BoxCollider> Create(const XMFLOAT3& center, const XMFLOAT3& size);

    private:
        void UpdateWorldBounds();
        bool IntersectsBox(const BoxCollider* other) const;
        bool IntersectsSphere(const SphereCollider* other) const;

        XMFLOAT3 m_size = { 1.0f, 1.0f, 1.0f };
        XMFLOAT3 m_worldMin = { -0.5f, -0.5f, -0.5f };
        XMFLOAT3 m_worldMax = { 0.5f, 0.5f, 0.5f };
    };

} // namespace Engine
