#pragma once

#include "box_collider.h"
#include <vector>
#include <unordered_map>

namespace Engine {

    class MapCollision {
    public:
        static MapCollision& GetInstance();

        void Initialize(float cellSize = 2.0f);
        void Shutdown();

        void RegisterBlock(BoxCollider* collider);
        void Clear();

        std::vector<BoxCollider*> GetNearbyBlocks(const XMFLOAT3& position, float radius);

        bool CheckCollision(BoxCollider* movingCollider, XMFLOAT3& outPenetration);
        std::vector<XMFLOAT3> CheckCollisionAll(BoxCollider* movingCollider, float checkRadius = 3.0f);

    private:
        MapCollision() = default;

        int64_t GetCellKey(int x, int y, int z) const;
        void GetCellCoord(const XMFLOAT3& pos, int& outX, int& outY, int& outZ) const;

        float m_cellSize = 2.0f;
        std::unordered_map<int64_t, std::vector<BoxCollider*>> m_grid;
    };

} // namespace Engine
