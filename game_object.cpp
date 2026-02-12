// game_object.cpp
#include "game_object.h"
#include <d3d11.h>

GameObject::GameObject()
    : position(0.0f, 0.0f, 0.0f)
    , scale(1.0f, 1.0f, 1.0f)
    , rotation(0.0f, 0.0f, 0.0f)
    , colliderType(ColliderType::None)
    , meshVertices(nullptr)
    , meshVertexCount(0)
    , texture(nullptr)
    , vertexBuffer(nullptr)
    , bufferNeedsUpdate(true) {
}

GameObject::~GameObject() {
    if (vertexBuffer) {
        vertexBuffer->Release();
        vertexBuffer = nullptr;
    }
}

void GameObject::setBoxCollider(const XMFLOAT3& size) {
    boxCollider = std::make_unique<BoxCollider>(
        BoxCollider::fromCenterAndSize(position, size)
    );
    colliderType = ColliderType::Box;
}

void GameObject::setMesh(const VERTEX_3D* vertices, int count, ID3D11ShaderResourceView* tex) {
    meshVertices = vertices;
    meshVertexCount = count;
    texture = tex;
    bufferNeedsUpdate = true; // メッシュが変わったのでバッファ更新フラグを立てる
}

void GameObject::createVertexBuffer() {
    if (vertexBuffer) {
        vertexBuffer->Release();
        vertexBuffer = nullptr;
    }

    if (!meshVertices || meshVertexCount == 0) return;

    D3D11_BUFFER_DESC bd = {};
    bd.Usage = D3D11_USAGE_DEFAULT; // DYNAMIC から DEFAULT に変更（より高速）
    bd.ByteWidth = sizeof(VERTEX_3D) * meshVertexCount;
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    bd.CPUAccessFlags = 0; // CPU からのアクセスは不要

    D3D11_SUBRESOURCE_DATA subResource = {};
    subResource.pSysMem = meshVertices;

    HRESULT hr = GetDevice()->CreateBuffer(&bd, &subResource, &vertexBuffer);
    if (FAILED(hr)) {
        vertexBuffer = nullptr;
    }
}

void GameObject::draw() {
    if (!meshVertices || meshVertexCount == 0) return;

    // バッファが必要な場合のみ作成/更新
    if (bufferNeedsUpdate || !vertexBuffer) {
        createVertexBuffer();
        bufferNeedsUpdate = false;
    }

    if (!vertexBuffer) return;

    // ワールド行列の計算（変更があった場合のみ）
    static XMFLOAT3 lastPosition = { 0, 0, 0 };
    static XMFLOAT3 lastRotation = { 0, 0, 0 };
    static XMFLOAT3 lastScale = { 1, 1, 1 };
    static XMMATRIX lastWorldMatrix = XMMatrixIdentity();

    bool transformChanged = (position.x != lastPosition.x || position.y != lastPosition.y || position.z != lastPosition.z ||
        rotation.x != lastRotation.x || rotation.y != lastRotation.y || rotation.z != lastRotation.z ||
        scale.x != lastScale.x || scale.y != lastScale.y || scale.z != lastScale.z);

    if (transformChanged) {
        XMMATRIX ScalingMatrix = XMMatrixScaling(scale.x, scale.y, scale.z);
        XMMATRIX RotationMatrix = XMMatrixRotationRollPitchYaw(
            XMConvertToRadians(rotation.x),
            XMConvertToRadians(rotation.y),
            XMConvertToRadians(rotation.z)
        );
        XMMATRIX TranslationMatrix = XMMatrixTranslation(position.x, position.y, position.z);
        lastWorldMatrix = ScalingMatrix * RotationMatrix * TranslationMatrix;

        lastPosition = position;
        lastRotation = rotation;
        lastScale = scale;
    }

    SetWorldMatrix(lastWorldMatrix);

    if (texture) {
        GetDeviceContext()->PSSetShaderResources(0, 1, &texture);
    }

    UINT stride = sizeof(VERTEX_3D);
    UINT offset = 0;
    GetDeviceContext()->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);
    GetDeviceContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    MATERIAL mat = {};
    mat.Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
    SetMaterial(mat);

    GetDeviceContext()->Draw(meshVertexCount, 0);
}

bool GameObject::checkCollision(const GameObject& other) const {
    if (colliderType == ColliderType::Box && other.colliderType == ColliderType::Box) {
        return boxCollider->intersects(*other.boxCollider);
    }
    return false;
}