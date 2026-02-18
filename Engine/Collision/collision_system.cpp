#include "pch.h"
#include "collision_system.h"

namespace Engine {

    CollisionSystem& CollisionSystem::GetInstance() {
        static CollisionSystem instance;
        return instance;
    }

    void CollisionSystem::Initialize() {
        m_colliders.clear();
        m_nextId = 1;
    }

    void CollisionSystem::Shutdown() {
        m_colliders.clear();
        m_callback = nullptr;
    }

    uint32_t CollisionSystem::Register(Collider* collider, CollisionLayer layer, CollisionLayer mask, void* userData) {
        if (!collider) return 0;

        uint32_t id = m_nextId++;
        ColliderData data;
        data.collider = collider;
        data.layer = layer;
        data.mask = mask;
        data.userData = userData;
        data.id = id;
        data.enabled = true;

        m_colliders[id] = data;
        return id;
    }

    void CollisionSystem::Unregister(uint32_t id) {
        m_colliders.erase(id);
    }

    void CollisionSystem::SetEnabled(uint32_t id, bool enabled) {
        auto it = m_colliders.find(id);
        if (it != m_colliders.end()) {
            it->second.enabled = enabled;
        }
    }

    void CollisionSystem::Update() {
        if (!m_callback) return;

        std::vector<ColliderData*> active;
        for (auto& pair : m_colliders) {
            if (pair.second.enabled && pair.second.collider) {
                active.push_back(&pair.second);
            }
        }

        for (size_t i = 0; i < active.size(); ++i) {
            for (size_t j = i + 1; j < active.size(); ++j) {
                ColliderData* a = active[i];
                ColliderData* b = active[j];

                if (!HasFlag(a->mask, b->layer)) continue;
                if (!HasFlag(b->mask, a->layer)) continue;

                if (a->collider->Intersects(b->collider)) {
                    CollisionHit hit;
                    hit.dataA = a;
                    hit.dataB = b;

                    if (a->collider->GetType() == ColliderType::BOX &&
                        b->collider->GetType() == ColliderType::BOX) {
                        static_cast<BoxCollider*>(a->collider)->ComputePenetration(
                            static_cast<BoxCollider*>(b->collider), hit.penetration);
                    }

                    m_callback(hit);
                }
            }
        }
    }

} // namespace Engine
