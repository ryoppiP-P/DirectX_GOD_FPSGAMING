#include "pch.h"
#include "game_object.h"

namespace Game {

GameObject::GameObject()
    : position(0.0f, 0.0f, 0.0f)
    , velocity(0.0f, 0.0f, 0.0f)
    , scale(1.0f, 1.0f, 1.0f)
    , rotation(0.0f, 0.0f, 0.0f)
    , boxCollider(nullptr)
    , meshVertices(nullptr)
    , meshVertexCount(0)
    , texture(nullptr)
    , vertexBuffer(nullptr)
    , bufferNeedsUpdate(true)
    , m_tag(ObjectTag::NONE)
    , m_active(true)
    , m_deleteFlag(false)
    , id(0)
    , m_worldMatrixDirty(true) {
}

GameObject::~GameObject() {
    if (vertexBuffer) {
        vertexBuffer->Release();
        vertexBuffer = nullptr;
    }
}

GameObject::GameObject(GameObject&& other) noexcept
    : position(other.position)
    , velocity(other.velocity)
    , scale(other.scale)
    , rotation(other.rotation)
    , boxCollider(std::move(other.boxCollider))
    , meshVertices(other.meshVertices)
    , meshVertexCount(other.meshVertexCount)
    , texture(other.texture)
    , vertexBuffer(other.vertexBuffer)
    , bufferNeedsUpdate(other.bufferNeedsUpdate)
    , m_tag(other.m_tag)
    , m_active(other.m_active)
    , m_deleteFlag(other.m_deleteFlag)
    , id(other.id)
    , m_cachedWorldMatrix(other.m_cachedWorldMatrix)
    , m_worldMatrixDirty(other.m_worldMatrixDirty) {
    other.vertexBuffer = nullptr;
    other.meshVertices = nullptr;
    other.texture = nullptr;
}

GameObject& GameObject::operator=(GameObject&& other) noexcept {
    if (this != &other) {
        if (vertexBuffer) vertexBuffer->Release();

        position = other.position;
        velocity = other.velocity;
        scale = other.scale;
        rotation = other.rotation;
        boxCollider = std::move(other.boxCollider);
        meshVertices = other.meshVertices;
        meshVertexCount = other.meshVertexCount;
        texture = other.texture;
        vertexBuffer = other.vertexBuffer;
        bufferNeedsUpdate = other.bufferNeedsUpdate;
        m_tag = other.m_tag;
        m_active = other.m_active;
        m_deleteFlag = other.m_deleteFlag;
        id = other.id;
        m_cachedWorldMatrix = other.m_cachedWorldMatrix;
        m_worldMatrixDirty = other.m_worldMatrixDirty;

        other.vertexBuffer = nullptr;
        other.meshVertices = nullptr;
        other.texture = nullptr;
    }
    return *this;
}

void GameObject::setPosition(const XMFLOAT3& p) {
    position = p;
    m_worldMatrixDirty = true;
    bufferNeedsUpdate = true;
    updateColliderTransform();
}

void GameObject::setRotation(const XMFLOAT3& r) {
    rotation = r;
    m_worldMatrixDirty = true;
    bufferNeedsUpdate = true;
    updateColliderTransform();
}

void GameObject::updateColliderTransform() {
    if (boxCollider) {
        boxCollider->SetTransform(position, rotation, scale);
    }
}

void GameObject::setBoxCollider(const XMFLOAT3& size) {
    boxCollider = std::make_unique<Engine::BoxCollider>(position, size);
    boxCollider->SetTransform(position, rotation, scale);
}

void GameObject::setMesh(const Engine::Vertex3D* vertices, int count, ID3D11ShaderResourceView* tex) {
    meshVertices = vertices;
    meshVertexCount = count;
    texture = tex;
    bufferNeedsUpdate = true;
}

void GameObject::createVertexBuffer() {
    if (vertexBuffer) {
        vertexBuffer->Release();
        vertexBuffer = nullptr;
    }

    if (!meshVertices || meshVertexCount == 0) return;

    D3D11_BUFFER_DESC bd = {};
    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.ByteWidth = sizeof(Engine::Vertex3D) * meshVertexCount;
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;

    D3D11_SUBRESOURCE_DATA subResource = {};
    subResource.pSysMem = meshVertices;

    Engine::GetDevice()->CreateBuffer(&bd, &subResource, &vertexBuffer);
}

void GameObject::draw() {
    if (!meshVertices || meshVertexCount == 0) return;

    if (bufferNeedsUpdate || !vertexBuffer) {
        createVertexBuffer();
        bufferNeedsUpdate = false;
    }

    if (!vertexBuffer) return;

    if (m_worldMatrixDirty) {
        XMMATRIX S = XMMatrixScaling(scale.x, scale.y, scale.z);
        XMMATRIX R = XMMatrixRotationRollPitchYaw(
            XMConvertToRadians(rotation.x),
            XMConvertToRadians(rotation.y),
            XMConvertToRadians(rotation.z));
        XMMATRIX T = XMMatrixTranslation(position.x, position.y, position.z);
        m_cachedWorldMatrix = S * R * T;
        m_worldMatrixDirty = false;
    }

    Engine::Renderer::GetInstance().SetWorldMatrix(m_cachedWorldMatrix);

    if (texture) {
        Engine::GetDeviceContext()->PSSetShaderResources(0, 1, &texture);
    }

    UINT stride = sizeof(Engine::Vertex3D);
    UINT offset = 0;
    Engine::GetDeviceContext()->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);
    Engine::GetDeviceContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    Engine::MaterialData mat = {};
    mat.diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);

    auto* ctx = Engine::Renderer::GetInstance().GetContext();
    auto* buf = Engine::Renderer::GetInstance().GetMaterialBuffer();
    if (ctx && buf) {
        ctx->UpdateSubresource(buf, 0, nullptr, &mat, 0, 0);
    }

    Engine::GetDeviceContext()->Draw(meshVertexCount, 0);
}

void GameObject::Move(const XMFLOAT3& direction, float speed, float deltaTime) {
    position.x += direction.x * speed * deltaTime;
    position.y += direction.y * speed * deltaTime;
    position.z += direction.z * speed * deltaTime;
    m_worldMatrixDirty = true;
    bufferNeedsUpdate = true;
    updateColliderTransform();
}

} // namespace Game
