// game_object.h
#pragma once
#include <memory>
#include "System/Core/renderer.h"
#include "System/Graphics/vertex.h"
#include "System/Graphics/material.h"
#include <DirectXMath.h>

using namespace DirectX;

// AABB当たり判定用構造体
struct BoxCollider {
    XMFLOAT3 min, max;

    BoxCollider() : min{0.0f, 0.0f, 0.0f}, max{0.0f, 0.0f, 0.0f} {}
    BoxCollider(const XMFLOAT3& minVal, const XMFLOAT3& maxVal) : min(minVal), max(maxVal) {}

    static BoxCollider fromCenterAndSize(const XMFLOAT3& center, const XMFLOAT3& size) {
        XMFLOAT3 half = { size.x * 0.5f, size.y * 0.5f, size.z * 0.5f };
        return BoxCollider(
            { center.x - half.x, center.y - half.y, center.z - half.z },
            { center.x + half.x, center.y + half.y, center.z + half.z }
        );
    }

    bool intersects(const BoxCollider& other) const {
        if (max.x < other.min.x || min.x > other.max.x) return false;
        if (max.y < other.min.y || min.y > other.max.y) return false;
        if (max.z < other.min.z || min.z > other.max.z) return false;
        return true;
    }
};

enum class ColliderType {
    None,
    Box
};

class GameObject {
public:
    XMFLOAT3 position;
    XMFLOAT3 scale;
    XMFLOAT3 rotation;

    ColliderType colliderType;
    std::unique_ptr<BoxCollider> boxCollider;

    // 描画用
    const Engine::Vertex3D* meshVertices;
    int meshVertexCount;
    ID3D11ShaderResourceView* texture;

    // パフォーマンス改善：VertexBufferをキャッシュ
    ID3D11Buffer* vertexBuffer;
    bool bufferNeedsUpdate;

    GameObject();
    ~GameObject(); // デストラクタを追加

    void setBoxCollider(const XMFLOAT3& size);
    void setMesh(const Engine::Vertex3D* vertices, int count, ID3D11ShaderResourceView* tex);
    void draw();
    bool checkCollision(const GameObject& other) const;

    // 位置や回転が変わった時にフラグを立てる
    void markBufferForUpdate() { bufferNeedsUpdate = true; }
    
    // *** 追加 ***
    // BoxCollider を取得（nullptr の可能性あり）
    BoxCollider* GetBoxCollider() const { return boxCollider.get(); }

    uint32_t getId() const { return id; }
    void setId(uint32_t v) { id = v; }
    XMFLOAT3 getPosition() const { return position; }
    void setPosition(const XMFLOAT3& p) { position = p; markBufferForUpdate(); }
    XMFLOAT3 getRotation() const { return rotation; }
    void setRotation(const XMFLOAT3& r) { rotation = r; markBufferForUpdate(); }

private:
    uint32_t id = 0;
    void createVertexBuffer(); // VertexBuffer作成用ヘルパー関数
};