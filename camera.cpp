// camera.cpp
#include "camera.h"
#include "keyboard.h"
#include "mouse.h"
#include "game_controller.h"
#include "player.h"
#include "map.h"
#include "NetWork/network_manager.h"
#include <cmath>
#include <stdio.h>
#include <iostream>

// メインカメラのシングルトン
static Camera s_mainCamera = {
 XMFLOAT3(0.0f,0.0f, -4.0f),
 XMFLOAT3(0.0f,0.0f,1.0f),
 XMFLOAT3(0.0f,1.0f,0.0f),
45.0f,
0.5f,
1000.0f,
0.0f,
0.0f
};

Camera& GetMainCamera() {
	return s_mainCamera;
}

// プレイヤー初期化用（外部からは呼ばない）
void InitializePlayer(Map* map, ID3D11ShaderResourceView* texture) {
	InitializePlayers(map, texture);
}

// プレイヤー描画用
void DrawPlayer() {
	DrawPlayers();
}

// Accessor for external modules to get local player's GameObject
GameObject* GetLocalPlayerGameObject() {
	return GetActivePlayerGameObject();
}

// UpdatePlayer wrapper called from main.cpp
void UpdatePlayer() {
	UpdatePlayers();
}

// ネットワーク用：プレイヤー強制移動
void ForceUpdatePlayerPosition(const XMFLOAT3& pos, const XMFLOAT3& rot) {
	Player* activePlayer = GetActivePlayer();
	if (activePlayer) {
		activePlayer->ForceSetPosition(pos);
		activePlayer->ForceSetRotation(rot);
	}
}

void Camera::Update(void) {
	// CameraManagerに処理を委譲
	CameraManager::GetInstance().Update(1.0f / 60.0f);
}

// CameraManager実装
CameraManager* CameraManager::instance = nullptr;

CameraManager& CameraManager::GetInstance() {
	if (!instance) {
		instance = new CameraManager();
	}
	return *instance;
}

void CameraManager::Update(float deltaTime) {
	static bool controllerInitialized = false;
	if (!controllerInitialized) {
		GameController::Initialize();
		controllerInitialized = true;
	}

	// 定数定義
	constexpr float rotateSpeed = 2.0f;
	constexpr float mouseSensitivity = 0.2f;
	constexpr float stickSensitivity = 3.0f;
	constexpr float cameraDistance = 5.0f;
	constexpr float cameraHeight = 2.0f;

	// ========== マウス入力 ==========
	Mouse_State mouseState;
	Mouse_GetState(&mouseState);

	static bool wasRightButtonDown = false;
	if (mouseState.rightButton && !wasRightButtonDown) {
		if (mouseState.positionMode == MOUSE_POSITION_MODE_ABSOLUTE) {
			Mouse_SetMode(MOUSE_POSITION_MODE_RELATIVE);
		}
	} else if (!mouseState.rightButton && wasRightButtonDown) {
		if (mouseState.positionMode == MOUSE_POSITION_MODE_RELATIVE) {
			Mouse_SetMode(MOUSE_POSITION_MODE_ABSOLUTE);
		}
	}
	wasRightButtonDown = mouseState.rightButton;

	if (mouseState.positionMode == MOUSE_POSITION_MODE_RELATIVE) {
		if (mouseState.x != 0 || mouseState.y != 0) {
			rotation += mouseState.x * mouseSensitivity;
			pitch -= mouseState.y * mouseSensitivity;

			if (pitch > 89.0f) pitch = 89.0f;
			if (pitch < -89.0f) pitch = -89.0f;
		}
	}

	// ========== キーボード追加操作（矢印キー） ==========
	if (Keyboard_IsKeyDown(KK_LEFT)) {
		rotation -= rotateSpeed;
	}
	if (Keyboard_IsKeyDown(KK_RIGHT)) {
		rotation += rotateSpeed;
	}

	if (Keyboard_IsKeyDown(KK_UP)) {
		pitch += rotateSpeed;
		if (pitch > 89.0f) pitch = 89.0f;
	}
	if (Keyboard_IsKeyDown(KK_DOWN)) {
		pitch -= rotateSpeed;
		if (pitch < -89.0f) pitch = -89.0f;
	}

	// ========== GameController ==========
	{
		GamepadState pad;
		const float rightDead =0.12f; // ノイズ除去用しきい値
		bool applied = false;
		if (GameController::GetState(pad)) {
			if (fabsf(pad.rightStickX) > rightDead || fabsf(pad.rightStickY) > rightDead) {
				pitch -= pad.rightStickX * stickSensitivity;
				rotation += pad.rightStickY * stickSensitivity;
				applied = true;
			}
		}
		// フォールバック: GetState が rightStick を返さない/誤る環境向けに生値を直接読む
		if (!applied) {
			const int RAW_CENTER =32767;
			const float rawDeadNorm =0.12f; // 正規化後の閾値
			for (int id =0; id <16; ++id) {
				int test = GameController::GetGamepadValue(id,3);
				if (test ==32767) continue;
				int rawR = GameController::GetGamepadValue(id,4); // dwRpos
				int rawU = GameController::GetGamepadValue(id,5); // dwUpos
				int dx = rawR - RAW_CENTER;
				int dy = rawU - RAW_CENTER;
				float nx = (float)dx / (float)RAW_CENTER;
				float ny = (float)dy / (float)RAW_CENTER;
				if (fabsf(nx) > rawDeadNorm || fabsf(ny) > rawDeadNorm) {
					pitch -= nx * stickSensitivity;
					rotation += ny * stickSensitivity;
					applied = true;
					break;
				}
			}
		}
		if (applied) {
			if (pitch >89.0f) pitch =89.0f;
			if (pitch < -89.0f) pitch = -89.0f;
		}
	}

	// アクティブプレイヤーに基づいてカメラを更新
	PlayerManager& playerMgr = PlayerManager::GetInstance();
	UpdateCameraForPlayer(playerMgr.GetActivePlayerId());
}

void CameraManager::UpdateCameraForPlayer(int playerId) {
	Player* player = PlayerManager::GetInstance().GetPlayer(playerId);
	if (!player) return;

	// プレイヤーの回転をカメラの回転に同期
	XMFLOAT3 playerRot = player->GetRotation();
	if (player->GetViewMode() == ViewMode::FIRST_PERSON) {
		//1人称: カメラの向きにプレイヤーを合わせる（従来の挙動）
		playerRot.y = rotation;
		player->ForceSetRotation(playerRot);
	} else {
		//3人称: カメラの向きにプレイヤーを合わせる（マウスやスティックで視点を動かしたらプレイヤーが向く）
		playerRot.y = rotation;
		player->ForceSetRotation(playerRot);
	}

	// カメラの方向ベクトル計算
	float yawRad = XMConvertToRadians(rotation);
	float pitchRad = XMConvertToRadians(pitch);

	float cosYaw = cosf(yawRad);
	float sinYaw = sinf(yawRad);
	float cosPitch = cosf(pitchRad);
	float sinPitch = sinf(pitchRad);

	XMFLOAT3 forward = {
	sinYaw * cosPitch,
	sinPitch,
	cosYaw * cosPitch
	};

	XMFLOAT3 playerPos = player->GetPosition();
	ViewMode viewMode = player->GetViewMode();

	if (viewMode == ViewMode::THIRD_PERSON) {
		//3人称視点: プレイヤーの後方から見る
		constexpr float cameraDistance = 5.0f;
		constexpr float cameraHeight = 2.0f;

		s_mainCamera.position.x = playerPos.x - forward.x * cameraDistance;
		s_mainCamera.position.y = playerPos.y + cameraHeight - forward.y * cameraDistance;
		s_mainCamera.position.z = playerPos.z - forward.z * cameraDistance;

		// 注視点はプレイヤーの中心
		s_mainCamera.Atposition.x = playerPos.x;
		s_mainCamera.Atposition.y = playerPos.y + 1.0f;
		s_mainCamera.Atposition.z = playerPos.z;
	} else { // FIRST_PERSON
		//1人称視点: プレイヤーの目線
		s_mainCamera.position.x = playerPos.x;
		s_mainCamera.position.y = playerPos.y + 1.0f; //目の高さ
		s_mainCamera.position.z = playerPos.z;

		s_mainCamera.Atposition.x = s_mainCamera.position.x + forward.x;
		s_mainCamera.Atposition.y = s_mainCamera.position.y + forward.y;
		s_mainCamera.Atposition.z = s_mainCamera.position.z + forward.z;
	}
}

// グローバル関数
void InitializeCameraSystem() {
	// 初期化は特に必要なし（シングルトンが自動的に初期化される）
}

void UpdateCameraSystem() {
	CameraManager::GetInstance().Update(1.0f / 60.0f);
}