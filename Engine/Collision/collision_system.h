#pragma once

#include "collider.h"
#include "box_collider.h"
#include <vector>
#include <functional>
#include <unordered_map>

namespace Engine {

    enum class CollisionLayer : uint32_t {
        NONE = 0,
        PLAYER = 1 << 0,
        ENEMY = 1 << 1,
        PROJECTILE = 1 << 2,
        TRIGGER = 1 << 3,
        ALL = 0xFFFFFFFF
    };

    inline CollisionLayer operator|(CollisionLayer a, CollisionLayer b) {
        return static_cast<CollisionLayer>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
    }

    inline bool HasFlag(CollisionLayer value, CollisionLayer flag) {
        return (static_cast<uint32_t>(value) & static_cast<uint32_t>(flag)) != 0;
    }

    struct ColliderData {
        Collider* collider = nullptr;
        CollisionLayer layer = CollisionLayer::NONE;
        CollisionLayer mask = CollisionLayer::ALL;
        void* userData = nullptr;
        uint32_t id = 0;
        bool enabled = true;
    };

    struct CollisionHit {
        ColliderData* dataA = nullptr;
        ColliderData* dataB = nullptr;
        XMFLOAT3 penetration = { 0, 0, 0 };
    };

    using CollisionCallback = std::function<void(const CollisionHit&)>;

    class CollisionSystem {
    public:
        static CollisionSystem& GetInstance();

        void Initialize();
        void Shutdown();

        uint32_t Register(Collider* collider, CollisionLayer layer, CollisionLayer mask, void* userData);
        void Unregister(uint32_t id);
        void SetEnabled(uint32_t id, bool enabled);

        void Update();
        void SetCallback(CollisionCallback callback) { m_callback = std::move(callback); }

    private:
        CollisionSystem() = default;

        std::unordered_map<uint32_t, ColliderData> m_colliders;
        uint32_t m_nextId = 1;
        CollisionCallback m_callback;
    };

} // namespace Engine
