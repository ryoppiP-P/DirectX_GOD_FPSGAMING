// network_host.cpp - ホスト/サーバー側ネットワーク処理の実装
#include "pch.h"
#include "network_host.h"
#include "Game/Objects/game_object.h"
#include "Game/Objects/camera.h"
#include "Engine/Graphics/primitive.h"
#include "main.h"
#include <iostream>
#include <cstring>

NetworkHost::NetworkHost(UdpNetwork& net, std::mutex& sharedMutex)
    : m_net(net), m_mutex(sharedMutex) {
}

bool NetworkHost::has_clients() const {
    return !m_clients.empty();
}

// ホスト側パケット振り分け処理
void NetworkHost::process_packet(const char* buf, int len, const std::string& from_ip, int from_port,
                                  Game::GameObject* localPlayer, std::vector<std::shared_ptr<Game::GameObject>>& worldObjects) {
    if (len <= 0) return;
    uint8_t t = static_cast<uint8_t>(buf[0]);

    if (t == PKT_JOIN) {
        netlog_printf("HOST RECV JOIN from=%s:%d bytes=%d", from_ip.c_str(), from_port, len);
        handle_join(from_ip, from_port, worldObjects);
    } else if (t == PKT_INPUT) {
        if (len >= static_cast<int>(sizeof(PacketInput))) {
            PacketInput pi;
            memcpy(&pi, buf, sizeof(PacketInput));
            handle_input(pi, worldObjects);

            // 最終接触時間を更新
            std::lock_guard<std::mutex> lk(m_mutex);
            for (auto& client : m_clients) {
                if (client.playerId == pi.playerId) {
                    client.lastSeen = std::chrono::steady_clock::now();
                    break;
                }
            }
        }
    } else if (t == PKT_PING) {
        // 軽量ハンドリング
    } else if (t == PKT_STATE) {
        handle_client_state(buf, len, from_ip, from_port, worldObjects);
    }
}

// JOIN処理：新規クライアントの登録とID割り当て
void NetworkHost::handle_join(const std::string& from_ip, int from_port,
                               std::vector<std::shared_ptr<Game::GameObject>>& worldObjects) {
    // 重複クライアントチェック
    {
        std::lock_guard<std::mutex> lk(m_mutex);
        for (auto& c : m_clients) {
            if (c.ip == from_ip && c.port == from_port) {
                netlog_printf("HOST JOIN: Client %s:%d already exists - ignoring", from_ip.c_str(), from_port);
                return;
            }
        }
    }

    // 衝突しないプレイヤーIDを決定（ID=1はホスト予約、リモートは2以降）
    uint32_t assignedId = 0;
    {
        std::lock_guard<std::mutex> lk(m_mutex);
        uint32_t cand = m_nextPlayerId;
        if (cand <= 1) cand = 2;
        bool conflict = true;
        while (conflict) {
            conflict = false;
            for (const auto& c : m_clients) {
                if (c.playerId == cand) { conflict = true; break; }
            }
            if (conflict) { ++cand; continue; }
            for (const auto& go : worldObjects) {
                if (go && go->getId() == cand) { conflict = true; break; }
            }
            if (conflict) ++cand;
        }
        assignedId = cand;

        // クライアント登録
        ClientInfo ci;
        ci.ip = from_ip;
        ci.port = from_port;
        ci.playerId = assignedId;
        ci.lastSeen = std::chrono::steady_clock::now();
        m_clients.push_back(ci);
        m_nextPlayerId = assignedId + 1;
    }

    netlog_printf("HOST NEW CLIENT: %s:%d assigned id=%u (total clients: %zu)", from_ip.c_str(), from_port, assignedId, m_clients.size());

    // 既存GameObjectの再利用チェック
    bool existed = false;
    for (const auto& go : worldObjects) {
        if (go && go->getId() == assignedId) {
            existed = true;
            break;
        }
    }

    if (!existed) {
        auto newObj = std::make_shared<Game::GameObject>();
        newObj->setPosition({0.0f, 3.0f, 0.0f});
        newObj->setRotation({0.0f, 0.0f, 0.0f});
        newObj->setId(assignedId);
        newObj->setMesh(Box, 36, GetPolygonTexture());
        newObj->setBoxCollider({0.8f, 1.8f, 0.8f});
        worldObjects.push_back(newObj);
        netlog_printf("HOST: Created worldObject for client id=%u", assignedId);
    } else {
        netlog_printf("HOST: Reusing existing worldObject for client id=%u", assignedId);
    }

    // JOIN_ACK送信
    uint8_t reply[1 + 4];
    NetworkProtocol::build_join_ack(reply, assignedId);
    bool sendSuccess = m_net.send_to(from_ip, from_port, reply, static_cast<int>(1 + 4));
    netlog_printf("HOST SEND JOIN_ACK -> %s:%d id=%u success=%d", from_ip.c_str(), from_port, assignedId, sendSuccess ? 1 : 0);
}

// INPUT処理：クライアントからの入力を適用
void NetworkHost::handle_input(const PacketInput& pi,
                                std::vector<std::shared_ptr<Game::GameObject>>& worldObjects) {
    for (const auto& go : worldObjects) {
        if (go->getId() == pi.playerId) {
            auto pos = go->getPosition();
            pos.x += pi.moveX;
            pos.y += pi.moveY;
            pos.z += pi.moveZ;
            go->setPosition(pos);
            return;
        }
    }
}

// クライアントから送信されたSTATEをホスト側で処理
void NetworkHost::handle_client_state(const char* buf, int len, const std::string& from_ip, int from_port,
                                       std::vector<std::shared_ptr<Game::GameObject>>& worldObjects) {
    PacketStateHeader hdr;
    const ObjectState* entries = nullptr;
    if (!NetworkProtocol::parse_state_packet(buf, len, hdr, entries)) return;

    netlog_printf("HOST RECV STATE from %s:%d objectCount=%u", from_ip.c_str(), from_port, hdr.objectCount);

    for (uint32_t i = 0; i < hdr.objectCount; ++i) {
        const ObjectState& os = entries[i];
        netlog_printf("HOST RECV STATE: Processing client id=%u pos=(%.2f,%.2f,%.2f)",
            os.id, os.posX, os.posY, os.posZ);

        bool applied = false;
        for (const auto& go : worldObjects) {
            if (go->getId() == os.id) {
                XMFLOAT3 oldPos = go->getPosition();
                go->setPosition({os.posX, os.posY, os.posZ});
                go->setRotation({os.rotX, os.rotY, os.rotZ});
                applied = true;
                netlog_printf("HOST RECV STATE: Updated client id=%u from (%.2f,%.2f,%.2f) to (%.2f,%.2f,%.2f)",
                    os.id, oldPos.x, oldPos.y, oldPos.z, os.posX, os.posY, os.posZ);
                break;
            }
        }
        if (!applied) {
            auto newObj = std::make_shared<Game::GameObject>();
            newObj->setId(os.id);
            newObj->setPosition({os.posX, os.posY, os.posZ});
            newObj->setRotation({os.rotX, os.rotY, os.rotZ});
            newObj->setMesh(Box, 36, GetPolygonTexture());
            newObj->setBoxCollider({0.8f, 1.8f, 0.8f});
            worldObjects.push_back(newObj);
            netlog_printf("HOST RECV STATE: Created new client object id=%u pos=(%.2f,%.2f,%.2f)",
                os.id, os.posX, os.posY, os.posZ);
        }
        // クライアントのlastSeen更新
        std::lock_guard<std::mutex> lk(m_mutex);
        for (auto& client : m_clients) {
            if (client.playerId == os.id) {
                client.lastSeen = std::chrono::steady_clock::now();
                break;
            }
        }
    }
}

// ラウンドロビン方式で1クライアントずつSTATE送信（負荷分散）
void NetworkHost::send_state_round_robin(std::vector<std::shared_ptr<Game::GameObject>>* worldObjectsPtr) {
    std::lock_guard<std::mutex> lk(m_mutex);
    if (m_clients.empty() || worldObjectsPtr == nullptr) return;

    size_t idx = m_stateSendIndex % m_clients.size();
    ClientInfo& c = m_clients[idx];

    PacketStateHeader header;
    header.type = PKT_STATE;
    header.seq = m_seq++;
    header.objectCount = 1;

    std::vector<char> sendbuf;
    sendbuf.resize(sizeof(header) + sizeof(ObjectState));
    memcpy(sendbuf.data(), &header, sizeof(header));

    ObjectState os = {};
    os.id = c.playerId;
    bool found = false;
    for (const auto& go : *worldObjectsPtr) {
        if (go && go->getId() == os.id) {
            auto p = go->getPosition();
            os.posX = p.x; os.posY = p.y; os.posZ = p.z;
            auto rrot = go->getRotation();
            os.rotX = rrot.x; os.rotY = rrot.y; os.rotZ = rrot.z;
            found = true;
            break;
        }
    }
    if (!found) {
        os.posX = os.posY = os.posZ = 0.0f;
        os.rotX = os.rotY = os.rotZ = 0.0f;
    }
    memcpy(sendbuf.data() + sizeof(header), &os, sizeof(ObjectState));

    if (!m_net.send_to(c.ip, c.port, sendbuf.data(), static_cast<int>(sendbuf.size()))) {
        netlog_printf("HOST SEND FAILED: to %s:%d", c.ip.c_str(), c.port);
    }

    m_stateSendIndex = (m_stateSendIndex + 1) % (m_clients.empty() ? 1 : m_clients.size());
}

// 全クライアントへSTATEブロードキャスト
void NetworkHost::send_state_to_all(std::vector<std::shared_ptr<Game::GameObject>>& worldObjects) {
    std::lock_guard<std::mutex> lk(m_mutex);
    if (m_clients.empty()) {
        netlog_printf("HOST SEND_STATE: No clients connected");
        return;
    }

    std::vector<ObjectState> states;

    // ホストプレイヤー（ID=1）をローカルプレイヤーから取得
    Game::GameObject* localPlayer = Game::GetLocalPlayerGameObject();
    if (localPlayer && localPlayer->getId() == 1) {
        ObjectState os = {};
        os.id = 1;
        auto p = localPlayer->getPosition();
        auto r = localPlayer->getRotation();
        os.posX = p.x; os.posY = p.y; os.posZ = p.z;
        os.rotX = r.x; os.rotY = r.y; os.rotZ = r.z;
        states.push_back(os);
        netlog_printf("HOST SEND_STATE: Added host LOCAL player id=1 pos=(%.2f,%.2f,%.2f)", p.x, p.y, p.z);
    }

    // 各接続クライアントのオブジェクトを追加
    for (const auto& c : m_clients) {
        ObjectState os = {};
        os.id = c.playerId;
        bool found = false;
        for (const auto& go : worldObjects) {
            if (go && go->getId() == os.id) {
                auto p = go->getPosition(); auto r = go->getRotation();
                os.posX = p.x; os.posY = p.y; os.posZ = p.z;
                os.rotX = r.x; os.rotY = r.y; os.rotZ = r.z;
                found = true;
                netlog_printf("HOST SEND_STATE: Added client id=%u pos=(%.2f,%.2f,%.2f)", os.id, p.x, p.y, p.z);
                break;
            }
        }
        if (!found) {
            os.posX = os.posY = os.posZ = 0.0f;
            os.rotX = os.rotY = os.rotZ = 0.0f;
            netlog_printf("HOST SEND_STATE: Client id=%u not found in worldObjects, using default pos", os.id);
        }
        states.push_back(os);
    }

    if (states.empty()) {
        netlog_printf("HOST SEND_STATE: No states to send");
        return;
    }

    auto buf = NetworkProtocol::build_state_packet(m_seq, states);

    netlog_printf("HOST SEND_STATE: Broadcasting %zu states to %zu clients", states.size(), m_clients.size());
    for (const auto& c : m_clients) {
        bool sendSuccess = m_net.send_to(c.ip, c.port, buf.data(), static_cast<int>(buf.size()));
        netlog_printf("HOST SEND_STATE: Sent to client %s:%d (id=%u) success=%d",
            c.ip.c_str(), c.port, c.playerId, sendSuccess ? 1 : 0);
    }
}

// ホスト側フレーム同期
void NetworkHost::frame_sync(Game::GameObject* localPlayer,
                              std::vector<std::shared_ptr<Game::GameObject>>& worldObjects) {
    if (!localPlayer) {
        netlog_printf("HOST FRAMESYNC: localPlayer is NULL");
        return;
    }

    netlog_printf("FRAMESYNC: Starting - isHost=1 localPlayerID=%u", localPlayer->getId());

    // ID=1とID=2の状態を構築して送信
    auto build_states_for_ids = [&](const std::vector<int>& ids) {
        std::vector<ObjectState> states;
        for (int id : ids) {
            ObjectState os = {};
            os.id = static_cast<uint32_t>(id);
            bool found = false;
            if (localPlayer && localPlayer->getId() == static_cast<uint32_t>(id)) {
                auto p = localPlayer->getPosition();
                auto r = localPlayer->getRotation();
                os.posX = p.x; os.posY = p.y; os.posZ = p.z;
                os.rotX = r.x; os.rotY = r.y; os.rotZ = r.z;
                found = true;
            } else {
                for (const auto& go : worldObjects) {
                    if (!go) continue;
                    if (go->getId() == static_cast<uint32_t>(id)) {
                        auto p = go->getPosition();
                        auto r = go->getRotation();
                        os.posX = p.x; os.posY = p.y; os.posZ = p.z;
                        os.rotX = r.x; os.rotY = r.y; os.rotZ = r.z;
                        found = true;
                        break;
                    }
                }
            }
            if (!found) {
                os.posX = os.posY = os.posZ = 0.0f;
                os.rotX = os.rotY = os.rotZ = 0.0f;
            }
            states.push_back(os);
        }
        return states;
    };

    std::vector<int> targetIds = {1, 2};

    std::lock_guard<std::mutex> lk(m_mutex);
    if (m_clients.empty()) {
        netlog_printf("HOST FRAMESYNC: No clients to send to - SKIPPING");
        return;
    }

    auto states = build_states_for_ids(targetIds);
    auto buf = NetworkProtocol::build_state_packet(m_seq, states);

    netlog_printf("HOST FRAMESYNC: Broadcasting %zu states to %zu clients", states.size(), m_clients.size());
    for (const auto& c : m_clients) {
        bool sendSuccess = m_net.send_to(c.ip, c.port, buf.data(), static_cast<int>(buf.size()));
        netlog_printf("HOST FRAMESYNC: Sent to client %s:%d (id=%u) success=%d",
            c.ip.c_str(), c.port, c.playerId, sendSuccess ? 1 : 0);
    }
}
