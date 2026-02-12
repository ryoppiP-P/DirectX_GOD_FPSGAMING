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

void GameObject::setMesh(const Engine::Vertex3D* vertices, int count, ID3D11ShaderResourceView* tex) {
    meshVertices = vertices;
    meshVertexCount = count;
    texture = tex;
    bufferNeedsUpdate = true; // ���b�V�����ς�����̂Ńo�b�t�@�X�V�t���O�𗧂Ă�
}

void GameObject::createVertexBuffer() {
    if (vertexBuffer) {
        vertexBuffer->Release();
        vertexBuffer = nullptr;
    }

    if (!meshVertices || meshVertexCount == 0) return;

    D3D11_BUFFER_DESC bd = {};
    bd.Usage = D3D11_USAGE_DEFAULT; // DYNAMIC ���� DEFAULT �ɕύX�i��荂���j
    bd.ByteWidth = sizeof(Engine::Vertex3D) * meshVertexCount;
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    bd.CPUAccessFlags = 0; // CPU ����̃A�N�Z�X�͕s�v

    D3D11_SUBRESOURCE_DATA subResource = {};
    subResource.pSysMem = meshVertices;

    HRESULT hr = Engine::GetDevice()->CreateBuffer(&bd, &subResource, &vertexBuffer);
    if (FAILED(hr)) {
        vertexBuffer = nullptr;
    }
}

void GameObject::draw() {
    if (!meshVertices || meshVertexCount == 0) return;

    // �o�b�t�@���K�v�ȏꍇ�̂ݍ쐬/�X�V
    if (bufferNeedsUpdate || !vertexBuffer) {
        createVertexBuffer();
        bufferNeedsUpdate = false;
    }

    if (!vertexBuffer) return;

    // ���[���h�s��̌v�Z�i�ύX���������ꍇ�̂݁j
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

    Engine::Renderer::GetInstance().SetWorldMatrix(lastWorldMatrix);

    if (texture) {
        Engine::GetDeviceContext()->PSSetShaderResources(0, 1, &texture);
    }

    UINT stride = sizeof(Engine::Vertex3D);
    UINT offset = 0;
    Engine::GetDeviceContext()->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);
    Engine::GetDeviceContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    Engine::MaterialData mat = {};
    mat.diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
    {
        ID3D11DeviceContext* ctx = Engine::Renderer::GetInstance().GetContext();
        ID3D11Buffer* buf = Engine::Renderer::GetInstance().GetMaterialBuffer();
        if (ctx && buf) {
            ctx->UpdateSubresource(buf, 0, nullptr, &mat, 0, 0);
        }
    }

    Engine::GetDeviceContext()->Draw(meshVertexCount, 0);
}

bool GameObject::checkCollision(const GameObject& other) const {
    if (colliderType == ColliderType::Box && other.colliderType == ColliderType::Box) {
        return boxCollider->intersects(*other.boxCollider);
    }
    return false;
}