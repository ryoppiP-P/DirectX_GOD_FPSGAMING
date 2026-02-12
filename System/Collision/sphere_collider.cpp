#include "sphere_collider.h"
#include "box_collider.h"
#include <algorithm>
#include <cmath>

namespace Engine {
    SphereCollider::SphereCollider()
        : m_radius(1.0f)
        , m_worldRadius(1.0f) {
    }

    SphereCollider::SphereCollider(const XMFLOAT3& center, float radius)
        : m_radius(radius) {
        m_position = center;
        UpdateWorldRadius();
    }

    void SphereCollider::SetRadius(float radius) {
        m_radius = radius;
        UpdateWorldRadius();
    }

    void SphereCollider::SetTransform(const XMFLOAT3& position, const XMFLOAT3& rotation, const XMFLOAT3& scale) {
        m_position = position;
        m_rotation = rotation;
        m_scale = scale;
        UpdateWorldRadius();
    }

    void SphereCollider::UpdateWorldRadius() {
        // スケールの最大値を使用
        float maxScale = (std::max)({ m_scale.x, m_scale.y, m_scale.z });
        m_worldRadius = m_radius * maxScale;
    }

    XMFLOAT3 SphereCollider::GetCenter() const {
        return m_position;
    }

    void SphereCollider::GetBounds(XMFLOAT3& outMin, XMFLOAT3& outMax) const {
        outMin = { m_position.x - m_worldRadius, m_position.y - m_worldRadius, m_position.z - m_worldRadius };
        outMax = { m_position.x + m_worldRadius, m_position.y + m_worldRadius, m_position.z + m_worldRadius };
    }

    bool SphereCollider::Intersects(const Collider* pOther) const {
        if (!pOther) return false;

        switch (pOther->GetType()) {
        case ColliderType::SPHERE:
        {
            const auto* pSphere = static_cast<const SphereCollider*>(pOther);
            float dx = m_position.x - pSphere->m_position.x;
            float dy = m_position.y - pSphere->m_position.y;
            float dz = m_position.z - pSphere->m_position.z;
            float distSq = dx * dx + dy * dy + dz * dz;
            float radiusSum = m_worldRadius + pSphere->m_worldRadius;
            return distSq <= radiusSum * radiusSum;
        }
        case ColliderType::BOX:
        {
            // Box側の判定に委譲（将来実装）
            return pOther->Intersects(this);
        }
        default:
            return false;
        }
    }
}
