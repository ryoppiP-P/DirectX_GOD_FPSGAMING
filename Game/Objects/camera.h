#pragma once
#include "main.h"

namespace Game {

enum class CameraMode {
    THIRD_PERSON,  // TPS
    FIRST_PERSON   // FPS
};

class Camera {
public:
	XMFLOAT3 position;
	XMFLOAT3 Atposition;
	XMFLOAT3 Upvector;
	float fov;
	float nearclip;
	float farclip;
	float rotation;
	float pitch;

	// �X�V����
	void Update(void);
};

// ���C���J�����̎擾
Camera& GetMainCamera();

// �����v���C���[�Ή��̃J�����Ǘ�
class CameraManager {
private:
    static CameraManager* instance;
    float rotation;
    float pitch;
    
public:
  static CameraManager& GetInstance();
    
    void Update(float deltaTime);
    void UpdateCameraForPlayer(int playerId);
    
    float GetRotation() const { return rotation; }
    float GetPitch() const { return pitch; }
    void SetRotation(float rot) { rotation = rot; }
    void SetPitch(float p) { pitch = p; }
};

// グローバル関数
void InitializeCameraSystem();
void UpdateCameraSystem();

// プレイヤーアクセス用ブリッジ関数
class GameObject;
GameObject* GetLocalPlayerGameObject();
void UpdatePlayer();
void ForceUpdatePlayerPosition(const DirectX::XMFLOAT3& pos, const DirectX::XMFLOAT3& rot);

} // namespace Game