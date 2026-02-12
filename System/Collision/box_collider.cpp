#include "box_collider.h"
#include "sphere_collider.h"
#include <algorithm>
#include <cmath>

namespace Engine {
    BoxCollider::BoxCollider()
        : m_size(1.0f, 1.0f, 1.0f) {
        UpdateWorldBounds();
    }

    BoxCollider::BoxCollider(const XMFLOAT3& center, const XMFLOAT3& size)
        : m_size(size) {
        m_position = center;
        UpdateWorldBounds();
    }

    void BoxCollider::SetSize(const XMFLOAT3& size) {
        m_size = size;
        UpdateWorldBounds();
    }

    void BoxCollider::SetTransform(const XMFLOAT3& position, const XMFLOAT3& rotation, const XMFLOAT3& scale) {
        m_position = position;
        m_rotation = rotation;
        m_scale = scale;
        UpdateWorldBounds();
    }

    void BoxCollider::UpdateWorldBounds() {
        // スケール適用後のハーフサイズ
        float halfX = m_size.x * m_scale.x * 0.5f;
        float halfY = m_size.y * m_scale.y * 0.5f;
        float halfZ = m_size.z * m_scale.z * 0.5f;

        m_worldMin = { m_position.x - halfX, m_position.y - halfY, m_position.z - halfZ };
        m_worldMax = { m_position.x + halfX, m_position.y + halfY, m_position.z + halfZ };
    }

    XMFLOAT3 BoxCollider::GetCenter() const {
        return m_position;
    }

    void BoxCollider::GetBounds(XMFLOAT3& outMin, XMFLOAT3& outMax) const {
        outMin = m_worldMin;
        outMax = m_worldMax;
    }

    bool BoxCollider::Intersects(const Collider* pOther) const {
        if (!pOther) return false;

        switch (pOther->GetType()) {
        case ColliderType::BOX:
            return IntersectsBox(static_cast<const BoxCollider*>(pOther));
        case ColliderType::SPHERE:
            return IntersectsSphere(static_cast<const SphereCollider*>(pOther));
        default:
            return false;
        }
    }

    bool BoxCollider::IntersectsBox(const BoxCollider* pOther) const {
        return (m_worldMin.x <= pOther->m_worldMax.x && m_worldMax.x >= pOther->m_worldMin.x) &&
            (m_worldMin.y <= pOther->m_worldMax.y && m_worldMax.y >= pOther->m_worldMin.y) &&
            (m_worldMin.z <= pOther->m_worldMax.z && m_worldMax.z >= pOther->m_worldMin.z);
    }

    bool BoxCollider::IntersectsSphere(const SphereCollider* pOther) const {
        // Box-Sphere判定（将来実装）
        // 今はfalseを返す
        return false;
    }

    bool BoxCollider::Contains(const XMFLOAT3& point) const {
        return (point.x >= m_worldMin.x && point.x <= m_worldMax.x) &&
            (point.y >= m_worldMin.y && point.y <= m_worldMax.y) &&
            (point.z >= m_worldMin.z && point.z <= m_worldMax.z);
    }

    std::unique_ptr<BoxCollider> BoxCollider::CreateFromCenterAndSize(const XMFLOAT3& center, const XMFLOAT3& size) {
        return std::make_unique<BoxCollider>(center, size);
    }
}
