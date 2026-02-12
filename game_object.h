// game_object.h
#pragma once
#include <memory>
#include "box_collider.h"
#include "renderer.h"

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
    const VERTEX_3D* meshVertices;
    int meshVertexCount;
    ID3D11ShaderResourceView* texture;

    // パフォーマンス改善：VertexBufferをキャッシュ
    ID3D11Buffer* vertexBuffer;
    bool bufferNeedsUpdate;

    GameObject();
    ~GameObject(); // デストラクタを追加

    void setBoxCollider(const XMFLOAT3& size);
    void setMesh(const VERTEX_3D* vertices, int count, ID3D11ShaderResourceView* tex);
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