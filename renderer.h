/*********************************************************************
  \file    �����_���[ [renderer.h]

  DirectX11 �����_���[�̃O���[�o���֐��Ɗ֘A�^��`�B
  ���̃w�b�_�[�͒P�̂Ŏg�p�ł��ASystem/ ���̃w�b�_�[�ɈˑR���Ȃ��B
 *********************************************************************/
#pragma once

#include <d3d11.h>
#include <DirectXMath.h>

// 3D ���_�\����
struct VERTEX_3D {
    DirectX::XMFLOAT3 position;
    DirectX::XMFLOAT3 normal;
    DirectX::XMFLOAT4 color;
    DirectX::XMFLOAT2 texCoord;

    VERTEX_3D()
        : position(0.0f, 0.0f, 0.0f)
        , normal(0.0f, 0.0f, 0.0f)
        , color(1.0f, 1.0f, 1.0f, 1.0f)
        , texCoord(0.0f, 0.0f) {
    }

    VERTEX_3D(const DirectX::XMFLOAT3& pos, const DirectX::XMFLOAT3& norm,
        const DirectX::XMFLOAT4& col, const DirectX::XMFLOAT2& uv)
        : position(pos)
        , normal(norm)
        , color(col)
        , texCoord(uv) {
    }
};

// MATERIAL �\����
struct MATERIAL {
    DirectX::XMFLOAT4 Diffuse;
    DirectX::XMFLOAT4 Ambient;
    DirectX::XMFLOAT4 Specular;
    DirectX::XMFLOAT4 Emission;
    float Shininess;
    float Padding[3];
};

// �f�o�C�X�A�N�Z�X
ID3D11Device* GetDevice();
ID3D11DeviceContext* GetDeviceContext();

// �����_���[������/�I��
HRESULT InitRenderer(HINSTANCE hInstance, HWND hWnd, BOOL bWindow);
void UninitRenderer();

// �t���[������
void Clear();
void Present();

// �s��ݒ�
void SetWorldMatrix(const DirectX::XMMATRIX& matrix);
void SetViewMatrix(const DirectX::XMMATRIX& matrix);
void SetProjectionMatrix(const DirectX::XMMATRIX& matrix);

// �[�x�ݒ�
void SetDepthEnable(bool enable);

// �}�e���A���ݒ�
void SetMaterial(const MATERIAL& mat);
