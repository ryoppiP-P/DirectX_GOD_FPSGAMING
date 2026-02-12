#pragma once
#include "main.h"

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

	// 更新処理
	void Update(void);
};

// メインカメラの取得
Camera& GetMainCamera();

// 複数プレイヤー対応のカメラ管理
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