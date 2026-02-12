#include "player.h"

// Definitions to resolve external linkage for PlayerManager accessors
Player* PlayerManager::GetActivePlayer() {
 if (activePlayerId ==1 && player1Initialized) return &player1;
 if (activePlayerId ==2 && player2Initialized) return &player2;
 return nullptr;
}

Player* PlayerManager::GetPlayer(int playerId) {
 if (playerId ==1 && player1Initialized) return &player1;
 if (playerId ==2 && player2Initialized) return &player2;
 return nullptr;
}
