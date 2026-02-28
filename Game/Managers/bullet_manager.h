/*********************************************************************
  \file    弾マネージャー [bullet_manager.h]

  \Author  Ryoto Kikuchi
  \data    2025
 *********************************************************************/
#pragma once

#include "Game/Objects/bullet.h"
#include <memory>
#include <vector>

namespace Game {

    class Player;

    class BulletManager {
    private:
        std::vector<std::unique_ptr<Bullet>> m_bullets;

        BulletManager() = default;

    public:
        static BulletManager& GetInstance() {
            static BulletManager instance;
            return instance;
        }

        void Add(std::unique_ptr<Bullet> bullet) {
            m_bullets.push_back(std::move(bullet));
        }

        void Update(float deltaTime) {
            for (auto& b : m_bullets) {
                b->Update(deltaTime);
            }

            // ★ CollisionSystem のコールバックが動かない場合のフォールバック
            //    弾の collider と Player の collider を直接チェック
            CheckBulletPlayerHits();

            m_bullets.erase(
                std::remove_if(m_bullets.begin(), m_bullets.end(),
                    [](const std::unique_ptr<Bullet>& b) { return !b->active; }),
                m_bullets.end());
        }

        void Draw() {
            for (auto& b : m_bullets) {
                b->Draw();
            }
        }

        void Clear() {
            m_bullets.clear();
        }

        size_t Count() const { return m_bullets.size(); }

        BulletManager(const BulletManager&) = delete;
        BulletManager& operator=(const BulletManager&) = delete;

    private:
        void CheckBulletPlayerHits();
    };

} // namespace Game
