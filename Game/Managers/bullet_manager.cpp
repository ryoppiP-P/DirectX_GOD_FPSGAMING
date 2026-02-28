/*********************************************************************
  \file    弾マネージャー [bullet_manager.cpp]

  \Author  Ryoto Kikuchi
  \data    2025
 *********************************************************************/
#include "pch.h"
#include "bullet_manager.h"
#include "Game/Managers/player_manager.h"
#include "Game/Objects/player.h"
#include <iostream>

namespace Game {

    void BulletManager::CheckBulletPlayerHits() {
        // 全プレイヤーIDをチェック（現在は1と2）
        for (int pid = 1; pid <= 2; ++pid) {
            Player* player = PlayerManager::GetInstance().GetPlayer(pid);
            if (!player || !player->IsAlive()) continue;

            Engine::BoxCollider* playerCol = player->GetColliderPtr();
            if (!playerCol) continue;

            for (auto& b : m_bullets) {
                if (!b || !b->active) continue;

                // 自分の弾には当たらない
                if (b->ownerPlayerId == player->GetPlayerId()) continue;

                // 弾の collider とプレイヤーの collider で直接 AABB 交差判定
                if (b->collider.Intersects(playerCol)) {
                    b->Deactivate();
                    player->TakeDamage(1);

                    std::cout << "[BulletManager Hit!] Player " << player->GetPlayerId()
                        << " HP=" << player->GetHP() << "/" << player->GetMaxHP() << "\n";

                    if (!player->IsAlive()) {
                        std::cout << "[BulletManager Kill!] Player " << player->GetPlayerId()
                            << " eliminated!\n";
                    }
                    break;  // この弾は消えたので次の弾へ
                }
            }
        }
    }

} // namespace Game
