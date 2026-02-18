/*********************************************************************
  \file    プレイヤーマネージャー [player_manager.h]
  
  \Author  Ryoto Kikuchi
  \data    2025
 *********************************************************************/
#pragma once

#include "Game/Objects/player.h"
#include "Game/Objects/camera.h"
#include <memory>
#include <vector>

namespace Game {

class Map;
class Bullet;

class PlayerManager {
private:
    static PlayerManager* instance;
    Player player1;
    Player player2;
    int activePlayerId;
    bool player1Initialized;
    bool player2Initialized;
    bool initialPlayerLocked;

    PlayerManager();

public:
    static PlayerManager& GetInstance();

    void Initialize(Map* map, ID3D11ShaderResourceView* texture);
    void SetInitialActivePlayer(int playerId);
    void Update(float deltaTime);
    void Draw();

    void SetActivePlayer(int playerId);
    int GetActivePlayerId() const { return activePlayerId; }

    Player* GetActivePlayer();
    Player* GetPlayer(int playerId);

    void HandleInput(float deltaTime);
};

void InitializePlayers(Map* map, ID3D11ShaderResourceView* texture);
void UpdatePlayers();
void DrawPlayers();
GameObject* GetActivePlayerGameObject();
Player* GetActivePlayer();

} // namespace Game
