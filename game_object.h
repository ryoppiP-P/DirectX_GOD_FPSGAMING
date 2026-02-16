#pragma once

#include <memory>
#include "System/Core/renderer.h"
#include "System/Graphics/vertex.h"
#include "System/Collision/box_collider.h"
#include "System/Graphics/material.h"
#include <DirectXMath.h>

using namespace DirectX;

class GameObject {
public:
    XMFLOAT3 position;
    XMFLOAT3 scale;
    XMFLOAT3 rotation;

    std::unique_ptr<Engine::BoxCollider> boxCollider;

    const Engine::Vertex3D* meshVertices;
    int meshVertexCount;
    ID3D11ShaderResourceView* texture;

    ID3D11Buffer* vertexBuffer;
    bool bufferNeedsUpdate;

    GameObject();
    ~GameObject();

    GameObject(const GameObject&) = delete;
    GameObject& operator=(const GameObject&) = delete;
    GameObject(GameObject&& other) noexcept;
    GameObject& operator=(GameObject&& other) noexcept;

    void setBoxCollider(const XMFLOAT3& size);
    void setMesh(const Engine::Vertex3D* vertices, int count, ID3D11ShaderResourceView* tex);
    void draw();

    void markBufferForUpdate() { bufferNeedsUpdate = true; m_worldMatrixDirty = true; }

    Engine::BoxCollider* GetBoxCollider() const { return boxCollider.get(); }

    uint32_t getId() const { return id; }
    void setId(uint32_t v) { id = v; }

    XMFLOAT3 getPosition() const { return position; }
    void setPosition(const XMFLOAT3& p);

    XMFLOAT3 getRotation() const { return rotation; }
    void setRotation(const XMFLOAT3& r);

private:
    uint32_t id = 0;
    XMMATRIX m_cachedWorldMatrix = XMMatrixIdentity();
    bool m_worldMatrixDirty = true;

    void createVertexBuffer();
    void updateColliderTransform();
};
