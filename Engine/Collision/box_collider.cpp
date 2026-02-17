#include "box_collider.h"
#include "sphere_collider.h"
#include <algorithm>
#include <cmath>

namespace Engine {

    BoxCollider::BoxCollider() : m_size(1.0f, 1.0f, 1.0f) {
        m_position = { 0.0f, 0.0f, 0.0f };
        UpdateWorldBounds();
    }

    BoxCollider::BoxCollider(const XMFLOAT3& center, const XMFLOAT3& size) : m_size(size) {
        m_position = center;
        UpdateWorldBounds();
    }

    BoxCollider::BoxCollider(const BoxCollider& other)
        : m_size(other.m_size)
        , m_worldMin(other.m_worldMin)
        , m_worldMax(other.m_worldMax) {
        m_position = other.m_position;
        m_rotation = other.m_rotation;
        m_scale = other.m_scale;
    }

    BoxCollider& BoxCollider::operator=(const BoxCollider& other) {
        if (this != &other) {
            m_position = other.m_position;
            m_rotation = other.m_rotation;
            m_scale = other.m_scale;
            m_size = other.m_size;
            m_worldMin = other.m_worldMin;
            m_worldMax = other.m_worldMax;
        }
        return *this;
    }

    void BoxCollider::SetCenter(const XMFLOAT3& center) {
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

    bool BoxCollider::Intersects(const Collider* other) const {
        if (!other) return false;

        switch (other->GetType()) {
        case ColliderType::BOX:
            return IntersectsBox(static_cast<const BoxCollider*>(other));
        case ColliderType::SPHERE:
            return IntersectsSphere(static_cast<const SphereCollider*>(other));
        default:
            return false;
        }
    }

    bool BoxCollider::IntersectsBox(const BoxCollider* other) const {
        return (m_worldMin.x <= other->m_worldMax.x && m_worldMax.x >= other->m_worldMin.x) &&
            (m_worldMin.y <= other->m_worldMax.y && m_worldMax.y >= other->m_worldMin.y) &&
            (m_worldMin.z <= other->m_worldMax.z && m_worldMax.z >= other->m_worldMin.z);
    }

    bool BoxCollider::IntersectsSphere(const SphereCollider* other) const {
        if (!other) return false;

        XMFLOAT3 center = other->GetCenter();
        float radius = other->GetWorldRadius();

        float closestX = std::clamp(center.x, m_worldMin.x, m_worldMax.x);
        float closestY = std::clamp(center.y, m_worldMin.y, m_worldMax.y);
        float closestZ = std::clamp(center.z, m_worldMin.z, m_worldMax.z);

        float dx = center.x - closestX;
        float dy = center.y - closestY;
        float dz = center.z - closestZ;

        return (dx * dx + dy * dy + dz * dz) <= (radius * radius);
    }

    bool BoxCollider::Contains(const XMFLOAT3& point) const {
        return (point.x >= m_worldMin.x && point.x <= m_worldMax.x) &&
            (point.y >= m_worldMin.y && point.y <= m_worldMax.y) &&
            (point.z >= m_worldMin.z && point.z <= m_worldMax.z);
    }

    bool BoxCollider::ComputePenetration(const BoxCollider* other, XMFLOAT3& outPenetration) const {
        if (!IntersectsBox(other)) {
            outPenetration = { 0.0f, 0.0f, 0.0f };
            return false;
        }

        float overlapX = std::min(m_worldMax.x - other->m_worldMin.x, other->m_worldMax.x - m_worldMin.x);
        float overlapY = std::min(m_worldMax.y - other->m_worldMin.y, other->m_worldMax.y - m_worldMin.y);
        float overlapZ = std::min(m_worldMax.z - other->m_worldMin.z, other->m_worldMax.z - m_worldMin.z);

        outPenetration = { 0.0f, 0.0f, 0.0f };

        if (overlapX <= overlapY && overlapX <= overlapZ) {
            float myCenter = (m_worldMin.x + m_worldMax.x) * 0.5f;
            float otherCenter = (other->m_worldMin.x + other->m_worldMax.x) * 0.5f;
            outPenetration.x = (myCenter < otherCenter) ? -overlapX : overlapX;
        } else if (overlapY <= overlapZ) {
            float myCenter = (m_worldMin.y + m_worldMax.y) * 0.5f;
            float otherCenter = (other->m_worldMin.y + other->m_worldMax.y) * 0.5f;
            outPenetration.y = (myCenter < otherCenter) ? -overlapY : overlapY;
        } else {
            float myCenter = (m_worldMin.z + m_worldMax.z) * 0.5f;
            float otherCenter = (other->m_worldMin.z + other->m_worldMax.z) * 0.5f;
            outPenetration.z = (myCenter < otherCenter) ? -overlapZ : overlapZ;
        }

        return true;
    }

    std::unique_ptr<BoxCollider> BoxCollider::Create(const XMFLOAT3& center, const XMFLOAT3& size) {
        return std::make_unique<BoxCollider>(center, size);
    }

} // namespace Engine
