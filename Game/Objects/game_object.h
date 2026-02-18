#pragma once

#include <memory>
#include "Engine/Core/renderer.h"
#include "Engine/Graphics/vertex.h"
#include "Engine/Collision/box_collider.h"
#include "Engine/Graphics/material.h"
#include <DirectXMath.h>

using namespace DirectX;

namespace Game {

// オブジェクト種別を識別する列挙型
enum class ObjectTag {
    NONE,
    PLAYER,
    ENEMY,
    BULLET,
    MAP_BLOCK,
    TRIGGER,
    CAMERA
};

class GameObject {
public:
    XMFLOAT3 position;
    XMFLOAT3 velocity;
    XMFLOAT3 scale;
    XMFLOAT3 rotation;

    std::unique_ptr<Engine::BoxCollider> boxCollider;

    const Engine::Vertex3D* meshVertices;
    int meshVertexCount;
    ID3D11ShaderResourceView* texture;

    ID3D11Buffer* vertexBuffer;
    bool bufferNeedsUpdate;

    GameObject();
    virtual ~GameObject();

    GameObject(const GameObject&) = delete;
    GameObject& operator=(const GameObject&) = delete;
    GameObject(GameObject&& other) noexcept;
    GameObject& operator=(GameObject&& other) noexcept;

    // ライフサイクル仮想関数
    virtual void Initialize() {}
    virtual void Finalize() {}
    virtual void Update(float deltaTime) {}
    virtual void OnCollision(GameObject* other) {}

    void setBoxCollider(const XMFLOAT3& size);
    void setMesh(const Engine::Vertex3D* vertices, int count, ID3D11ShaderResourceView* tex);
    virtual void draw();

    void markBufferForUpdate() { bufferNeedsUpdate = true; m_worldMatrixDirty = true; }

    Engine::BoxCollider* GetBoxCollider() const { return boxCollider.get(); }

    uint32_t getId() const { return id; }
    void setId(uint32_t v) { id = v; }

    XMFLOAT3 getPosition() const { return position; }
    void setPosition(const XMFLOAT3& p);

    XMFLOAT3 getVelocity() const { return velocity; }
    void setVelocity(const XMFLOAT3& v) { velocity = v; }

    XMFLOAT3 getRotation() const { return rotation; }
    void setRotation(const XMFLOAT3& r);

    XMFLOAT3 getScale() const { return scale; }

    // 共通ヘルパー関数
    void Move(const XMFLOAT3& direction, float speed, float deltaTime);
    bool IsActive() const { return m_active; }
    void SetActive(bool active) { m_active = active; }
    bool IsMarkedForDelete() const { return m_deleteFlag; }
    void MarkForDelete() { m_deleteFlag = true; }
    ObjectTag GetTag() const { return m_tag; }
    void SetTag(ObjectTag tag) { m_tag = tag; }

protected:
    ObjectTag m_tag = ObjectTag::NONE;
    bool m_active = true;
    bool m_deleteFlag = false;

private:
    uint32_t id = 0;
    XMMATRIX m_cachedWorldMatrix = XMMatrixIdentity();
    bool m_worldMatrixDirty = true;

    void createVertexBuffer();
    void updateColliderTransform();
};

} // namespace Game
