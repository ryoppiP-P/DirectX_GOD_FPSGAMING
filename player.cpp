// player.cpp
#include "player.h"
#include "map.h"
#include "map_renderer.h"
#include "System/Graphics/primitive.h"
#include "NetWork/network_manager.h"
#include "keyboard.h"
#include "camera.h" // �J���������̂��߂ɒǉ�
#include <cmath>
#include <algorithm>
#include "game_controller.h" // �R���g���[������

#include "bullet.h"
#include <memory>
#include <vector>
static std::vector<std::unique_ptr<Bullet>> g_bullets;



// min�̑���Ɏg���w���p�[�֐�
static inline float Min(float a, float b) {
    return (a < b) ? a : b;
}

static inline float Max(float a, float b) {
    return (a > b) ? a : b;
}

Player::Player()
    : position(0.0f, 3.0f, 0.0f)
    , velocity(0.0f, 0.0f, 0.0f)
    , rotation(0.0f, 0.0f, 0.0f)
    , scale(WIDTH, HEIGHT, DEPTH)
    , isGrounded(false)
    , mapRef(nullptr)
    , playerId(0)
    , viewMode(ViewMode::THIRD_PERSON)
    , cameraYaw(0.0f)
    , cameraPitch(0.0f)
    , hp(100)
    , isAlive(true)
{
    UpdateCollider();
}

void Player::Initialize(Map* map, ID3D11ShaderResourceView* texture, int id, ViewMode mode) {
    mapRef = map;
    playerId = id;
    viewMode = mode;

    // �v���C���[2�͏������ꂽ�ʒu�ɔz�u
    if (playerId == 2) {
        position = XMFLOAT3(3.0f, 3.0f, 0.0f);
    }

    // �r�W���A���I�u�W�F�N�g�̐ݒ�
    visualObject.position = position;
    visualObject.scale = scale;
    visualObject.rotation = rotation;
    visualObject.setMesh(Box, 36, texture);
    visualObject.setBoxCollider(scale);
    visualObject.markBufferForUpdate();
}

void Player::SetPosition(const XMFLOAT3& pos) {
    position = pos;
    UpdateCollider();
}

void Player::UpdateCollider() {
    collider = BoxCollider::fromCenterAndSize(position, scale);
    // GameObject��BoxCollider���Ǐ]������
    visualObject.position = position;
    visualObject.scale = scale;
    visualObject.setBoxCollider(scale);
    visualObject.markBufferForUpdate();
}

void Player::Update(float deltaTime) {
   
    // *** �ǉ� ***
    if (!isAlive) return;
    // *** �ǉ������܂� ***
    // �O��̈ʒu���L�^
    XMFLOAT3 previousPosition = position;

    // �d�͓K�p
    if (!isGrounded) {
        velocity.y += GRAVITY * deltaTime;
    }

    // ���x���ʒu�ɓK�p
    position.x += velocity.x * deltaTime;
    position.y += velocity.y * deltaTime;
    position.z += velocity.z * deltaTime;

    // �ʒu���ς�����ꍇ�̂݃R���C�_�[�X�V
    bool positionChanged = (fabsf(position.x - previousPosition.x) > 0.001f ||
        fabsf(position.y - previousPosition.y) > 0.001f ||
        fabsf(position.z - previousPosition.z) > 0.001f);

    if (positionChanged) {
        UpdateCollider();
    }

    // �}�b�v�Ƃ̏Փ˔���
    isGrounded = false;
    CheckMapCollision();

    // ���C
    velocity.x *= FRICTION;
    velocity.z *= FRICTION;

    // �ɏ��̑��x��0�ɂ���
    if (fabsf(velocity.x) < 0.01f) velocity.x = 0.0f;
    if (fabsf(velocity.z) < 0.01f) velocity.z = 0.0f;

    // �r�W���A���I�u�W�F�N�g�X�V�i�ʒu���ς�����ꍇ�̂݁j
    if (positionChanged) {
        visualObject.position = position;
        visualObject.rotation = rotation;
        visualObject.markBufferForUpdate();
    }
}

void Player::Move(const XMFLOAT3& direction, float deltaTime) {
    velocity.x = direction.x * MOVE_SPEED;
    velocity.z = direction.z * MOVE_SPEED;
}

void Player::Jump() {
    if (isGrounded) {
        velocity.y = JUMP_POWER;
        isGrounded = false;
    }
}

void Player::CheckMapCollision() {
    if (!mapRef) return;
    isGrounded = false;

    // �v���C���[�̋߂��̃u���b�N�̂݃`�F�b�N�i�e���œK���j
    const auto& blocks = mapRef->GetBlockObjects();
    const float checkRadius = 5.0f; // �v���C���[����5���j�b�g�ȓ��̃u���b�N�̂݃`�F�b�N

    for (const auto& blockPtr : blocks) {
        const auto& block = *blockPtr;

        // �����`�F�b�N�ɂ�鑁���J�b�g
        XMFLOAT3 blockPos = block.getPosition();
        float distanceSquared = (position.x - blockPos.x) * (position.x - blockPos.x) +
            (position.y - blockPos.y) * (position.y - blockPos.y) +
            (position.z - blockPos.z) * (position.z - blockPos.z);

        if (distanceSquared > checkRadius * checkRadius) {
            continue; // ��������u���b�N�̓X�L�b�v
        }

        if (block.colliderType == ColliderType::Box && block.boxCollider) {
            XMFLOAT3 penetration;
            if (visualObject.checkCollision(block)) {
                if (CheckCollisionWithBox(*block.boxCollider, penetration)) {
                    ResolveCollision(penetration);
                }
            }
        }
    }
}

bool Player::CheckCollisionWithBox(const BoxCollider& box, XMFLOAT3& penetration) {
    if (!collider.intersects(box)) {
        return false;
    }

    // �e���ł̏d�Ȃ�ʂ��v�Z�istd::min�̑����Min�֐����g�p�j
    float overlapX = Min(collider.max.x - box.min.x, box.max.x - collider.min.x);
    float overlapY = Min(collider.max.y - box.min.y, box.max.y - collider.min.y);
    float overlapZ = Min(collider.max.z - box.min.z, box.max.z - collider.min.z);

    // �ŏ��̏d�Ȃ莲��������
    if (overlapX < overlapY && overlapX < overlapZ) {
        // X���ŉ����߂�
        float boxCenterX = box.min.x + (box.max.x - box.min.x) * 0.5f;
        penetration.x = (position.x < boxCenterX) ? -overlapX : overlapX;
        penetration.y = 0.0f;
        penetration.z = 0.0f;
    } else if (overlapY < overlapZ) {
        // Y���ŉ����߂�
        float boxCenterY = box.min.y + (box.max.y - box.min.y) * 0.5f;
        penetration.x = 0.0f;
        penetration.y = (position.y < boxCenterY) ? -overlapY : overlapY;
        penetration.z = 0.0f;
    } else {
        // Z���ŉ����߂�
        float boxCenterZ = box.min.z + (box.max.z - box.min.z) * 0.5f;
        penetration.x = 0.0f;
        penetration.y = 0.0f;
        penetration.z = (position.z < boxCenterZ) ? -overlapZ : overlapZ;
    }

    return true;
}

void Player::ResolveCollision(const XMFLOAT3& penetration) {
    // �ʒu��␳
    position.x += penetration.x;
    position.y += penetration.y;
    position.z += penetration.z;

    // ���x��␳
    if (penetration.x != 0.0f) {
        velocity.x = 0.0f;
    }
    if (penetration.y != 0.0f) {
        velocity.y = 0.0f;
        // �ォ�牟���߂��ꂽ = �n�ʂɐڒn
        if (penetration.y > 0.0f) {
            isGrounded = true;
        }
    }
    if (penetration.z != 0.0f) {
        velocity.z = 0.0f;
    }

    // �R���C�_�[�X�V
    UpdateCollider();
}

void Player::Draw() {
    // *** �ǉ� ***
    if (!isAlive) return;
    // *** �ǉ������܂� ***
    visualObject.draw();
}

GameObject* Player::GetGameObject() {
    return &visualObject;
}

// �l�b�g���[�N�p�F�v���C���[�����ړ��i�n�ʒ����t���j
void Player::ForceSetPosition(const XMFLOAT3& pos) {
    position = pos;

    // �n�ʂ̍������m�F���Ē���
    if (mapRef) {
        float groundY = mapRef->GetGroundHeight(position.x, position.z);
        // Y���W���n�ʂ��Ⴂ�ꍇ�͒n�ʂɒ���
        if (position.y < groundY + scale.y * 0.5f) {
            position.y = groundY + scale.y * 0.5f;
        }
    }

    UpdateCollider();
}

void Player::ForceSetRotation(const XMFLOAT3& rot) {
    rotation = rot;
    visualObject.rotation = rotation;
    visualObject.markBufferForUpdate();
}

// �J�����p�x�֘A�̐ݒ�
void Player::SetCameraAngles(float yaw, float pitch) {
    cameraYaw = yaw;
    cameraPitch = pitch;
}

float Player::GetCameraYaw() const {
    return cameraYaw;
}

float Player::GetCameraPitch() const {
    return cameraPitch;
}

// NPC �p�F�Փ˗ʂ��v�Z����i�O��������S�ɌĂׂ�j
bool Player::ComputePenetrationWithBox(const BoxCollider& box, XMFLOAT3& penetration) {
    //�����ł̓v���C���[�̃R���C�_�[�Ǝw�� box �̏d�Ȃ�𔻒肵 penetration ��Ԃ�
    return CheckCollisionWithBox(box, penetration);
}

void Player::ApplyPenetration(const XMFLOAT3& penetration) {
    //������ResolveCollision �𗘗p���Ĉʒu�Ƒ��x���X�V
    ResolveCollision(penetration);
}

// PlayerManager����
PlayerManager* PlayerManager::instance = nullptr;

PlayerManager::PlayerManager()
    : activePlayerId(1)
    , player1Initialized(false)
    , player2Initialized(false)
    , initialPlayerLocked(false) {
}

PlayerManager& PlayerManager::GetInstance() {
    if (!instance) {
        instance = new PlayerManager();
    }
    return *instance;
}

void PlayerManager::SetInitialActivePlayer(int playerId) {
    if (playerId ==1 || playerId ==2) {
        initialPlayerLocked = true;
        activePlayerId = playerId;
    }
}

void PlayerManager::Initialize(Map* map, ID3D11ShaderResourceView* texture) {
    if (initialPlayerLocked) {
        // Initialize only the chosen player, do not create the other
        if (activePlayerId ==1) {
            if (!player1Initialized) {
                player1.Initialize(map, texture,1, ViewMode::THIRD_PERSON);
                player1.SetPosition(XMFLOAT3(0.0f,3.0f,0.0f));
                player1Initialized = true;
            }
            player2Initialized = false;
        } else { // activePlayerId ==2
            if (!player2Initialized) {
                player2.Initialize(map, texture,2, ViewMode::FIRST_PERSON);
                player2.SetPosition(XMFLOAT3(3.0f,3.0f,0.0f));
                player2Initialized = true;
            }
            player1Initialized = false;
        }
        // activePlayerId already set via SetInitialActivePlayer
        return;
    }

    if (!player1Initialized) {
        player1.Initialize(map, texture,1, ViewMode::THIRD_PERSON);
        player1.SetPosition(XMFLOAT3(0.0f,3.0f,0.0f));
        player1Initialized = true;
    }

    if (!player2Initialized) {
        player2.Initialize(map, texture,2, ViewMode::FIRST_PERSON);
        player2.SetPosition(XMFLOAT3(3.0f,3.0f,0.0f));
        player2Initialized = true;
    }

    activePlayerId =1; // �f�t�H���g�Ńv���C���[1
}

void PlayerManager::Update(float deltaTime) {
    // ���͏���
    HandleInput(deltaTime);

    // If initial player locked, update only active player; otherwise update both
    if (initialPlayerLocked) {
        Player* p = GetActivePlayer();
        if (p) p->Update(deltaTime);
        return;
    }

    // �����̃v���C���[�𕨗����Z�ōX�V�i�d�́A�Փ˔���Ȃǁj
    if (player1Initialized) {
        player1.Update(deltaTime);
    }
    if (player2Initialized) {
        player2.Update(deltaTime);
    }

    // *** �ǉ� ***
    // --- �e�̍X�V ---
    for (auto& b : g_bullets) {
        b->Update(deltaTime);

        // --- �e��Player1�ɓ��������ꍇ ---
        Player* player1Ptr = GetPlayer(1);
        if (player1Ptr && player1Ptr->IsAlive() && b->CheckHit(player1Ptr->GetCollider())) {
            b->Deactivate();
            player1Ptr->TakeDamage(50); // �_���[�W�ʒ�����
        }
    }

    // ���������ꂽ�e���폜
    g_bullets.erase(
        std::remove_if(g_bullets.begin(), g_bullets.end(),
            [](const std::unique_ptr<Bullet>& b) { return !b->active; }),
        g_bullets.end());

    // *** �ǉ������܂� ***
}

void PlayerManager::Draw() {
    if (initialPlayerLocked) {
        Player* p = GetActivePlayer();
        if (p) p->Draw();
        return;
    }

    // �����̃v���C���[��`��i�A�N�e�B�u�łȂ�����������悤�Ɂj
    if (player1Initialized) {
        player1.Draw();
    }
    if (player2Initialized) {
        player2.Draw();
    }

    // *** �ǉ� ***
    // �e�̕`��
    for (auto& b : g_bullets) {
        b->Draw();
    }
    // *** �ǉ������܂� ***
}

void PlayerManager::SetActivePlayer(int playerId) {
    if (initialPlayerLocked) return; // switching disabled when locked

    if (playerId ==1 || playerId ==2) {
        // ���݂̃A�N�e�B�u�v���C���[�̃J�����p�x��ۑ�
        Player* current = GetActivePlayer();
        CameraManager& camMgr = CameraManager::GetInstance();
        if (current) {
            current->SetCameraAngles(camMgr.GetRotation(), camMgr.GetPitch());
        }

        // �A�N�e�B�u�v���C���[��؂�ւ�
        activePlayerId = playerId;

        // �؂�ւ���v���C���[�̃J�����p�x�𕜌�
        Player* next = GetActivePlayer();
        if (next) {
            camMgr.SetRotation(next->GetCameraYaw());
            camMgr.SetPitch(next->GetCameraPitch());
            // �J�����ʒu�ƒ����_�𑦍��ɍX�V
            camMgr.UpdateCameraForPlayer(activePlayerId);
        }
    }
}

void PlayerManager::HandleInput(float deltaTime) {
    // If locked, ignore switch keys entirely
    static bool was1Down = false;
    static bool was2Down = false;

    if (!initialPlayerLocked) {
        if (Keyboard_IsKeyDown(KK_D1) && !was1Down) {
            SetActivePlayer(1);
        }
        if (Keyboard_IsKeyDown(KK_D2) && !was2Down) {
            SetActivePlayer(2);
        }
    }

    was1Down = Keyboard_IsKeyDown(KK_D1);
    was2Down = Keyboard_IsKeyDown(KK_D2);

    // �A�N�e�B�u�v���C���[�݂̂����͂��󂯕t����
    Player* activePlayer = GetActivePlayer();
    if (!activePlayer) return;

    XMFLOAT3 moveDirection = {0.0f,0.0f,0.0f };

    // �J�����̌������l�������ړ��v�Z
    float yawRad = XMConvertToRadians(activePlayer->GetRotation().y);
    XMFLOAT3 forward = { sinf(yawRad),0.0f, cosf(yawRad) };
    XMFLOAT3 right = { cosf(yawRad),0.0f, -sinf(yawRad) };

    if (Keyboard_IsKeyDown(KK_W)) {
        moveDirection.x += forward.x;
        moveDirection.z += forward.z;
    }
    if (Keyboard_IsKeyDown(KK_S)) {
        moveDirection.x -= forward.x;
        moveDirection.z -= forward.z;
    }
    if (Keyboard_IsKeyDown(KK_A)) {
        moveDirection.x -= right.x;
        moveDirection.z -= right.z;
    }
    if (Keyboard_IsKeyDown(KK_D)) {
        moveDirection.x += right.x;
        moveDirection.z += right.z;
    }

    // �R���g���[���̍��X�e�B�b�N�𓝍�
    GamepadState padState;
    if (GameController::GetState(padState)) {
        // ���X�e�B�b�N�̒l���擾
        float lx = padState.leftStickX; // ����-1�A�E��+1 �̑z��
        float ly = padState.leftStickY; // �O��+1�A�オ-1 �̑z��
        const float deadzone =0.2f;
        if (fabsf(lx) > deadzone || fabsf(ly) > deadzone) {
            // WASD �Ɠ��������ɂȂ�悤�Ƀ}�b�s���O
            // W/S -> forward * ly, A/D -> right * lx
            moveDirection.x += -forward.x * ly + right.x * lx;
            moveDirection.z += -forward.z * ly + right.z * lx;
        }
    }

    // �ړ��x�N�g���𐳋K��
    float moveLength = sqrtf(moveDirection.x * moveDirection.x + moveDirection.z * moveDirection.z);
    if (moveLength >0.0f) {
        moveDirection.x /= moveLength;
        moveDirection.z /= moveLength;
    }

    // �A�N�e�B�u�v���C���[�݂̂��ړ����͂��󂯕t����
    activePlayer->Move(moveDirection, deltaTime);

    // �W�����v���A�N�e�B�u�v���C���[�̂�
    if (Keyboard_IsKeyDown(KK_SPACE)) {
        activePlayer->Jump();
    }

    // *** �ǉ� ***
    // �e���ˁi2P��p�j
    if (activePlayer->GetPlayerId() == 2 && activePlayer->IsAlive() && Keyboard_IsKeyDownTrigger(KK_ENTER)) {
        XMFLOAT3 pos = activePlayer->GetPosition();
        float yawRad = XMConvertToRadians(activePlayer->GetRotation().y);
        XMFLOAT3 dir = { sinf(yawRad), 0.0f, cosf(yawRad) };

        auto b = std::make_unique<Bullet>();
        b->Initialize(GetPolygonTexture(), pos, dir);
        g_bullets.push_back(std::move(b));
    }
    // *** �ǉ������܂� ***
}

// �O���[�o���֐��i����݊����j
void InitializePlayers(Map* map, ID3D11ShaderResourceView* texture) {
    PlayerManager::GetInstance().Initialize(map, texture);
}

void UpdatePlayers() {
    constexpr float fixedDelta = 1.0f / 60.0f;
    PlayerManager::GetInstance().Update(fixedDelta);
}

void DrawPlayers() {
    PlayerManager::GetInstance().Draw();
}

GameObject* GetActivePlayerGameObject() {
    Player* activePlayer = PlayerManager::GetInstance().GetActivePlayer();
    return activePlayer ? activePlayer->GetGameObject() : nullptr;
}

Player* GetActivePlayer() {
    return PlayerManager::GetInstance().GetActivePlayer();
}