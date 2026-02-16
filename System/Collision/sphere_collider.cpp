#include "sphere_collider.h"
#include "box_collider.h"
#include <algorithm>
#include <cmath>

namespace Engine {

    SphereCollider::SphereCollider() : m_radius(1.0f), m_worldRadius(1.0f) {
        m_position = { 0.0f, 0.0f, 0.0f };
    }

    SphereCollider::SphereCollider(const XMFLOAT3& center, float radius) : m_radius(radius) {
        m_position = center;
        UpdateWorldRadius();
    }

    void SphereCollider::SetCenter(const XMFLOAT3& center) {
        m_position = center;
    }

    void SphereCollider::SetRadius(float radius) {
        m_radius = radius;
        UpdateWorldRadius();
    }

    void SphereCollider::UpdateWorldRadius() {
        float maxScale = std::max({ m_scale.x, m_scale.y, m_scale.z });
        m_worldRadius = m_radius * maxScale;
    }

    XMFLOAT3 SphereCollider::GetCenter() const {
        return m_position;
    }

    void SphereCollider::GetBounds(XMFLOAT3& outMin, XMFLOAT3& outMax) const {
        outMin = { m_position.x - m_worldRadius, m_position.y - m_worldRadius, m_position.z - m_worldRadius };
        outMax = { m_position.x + m_worldRadius, m_position.y + m_worldRadius, m_position.z + m_worldRadius };
    }

    bool SphereCollider::Intersects(const Collider* other) const {
        if (!other) return false;

        switch (other->GetType()) {
        case ColliderType::SPHERE: {
            const auto* s = static_cast<const SphereCollider*>(other);
            float dx = m_position.x - s->m_position.x;
            float dy = m_position.y - s->m_position.y;
            float dz = m_position.z - s->m_position.z;
            float distSq = dx * dx + dy * dy + dz * dz;
            float radiusSum = m_worldRadius + s->m_worldRadius;
            return distSq <= radiusSum * radiusSum;
        }
        case ColliderType::BOX:
            return other->Intersects(this);
        default:
            return false;
        }
    }

} // namespace Engine
