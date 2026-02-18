// network_client.cpp - クライアント側ネットワーク処理の実装
#include "pch.h"
#include "network_client.h"
#include "Game/Objects/game_object.h"
#include "Engine/Graphics/primitive.h"
#include <iostream>
#include <cstring>
#include <chrono>

NetworkClient::NetworkClient(UdpNetwork& net, UdpNetwork& discovery)
    : m_net(net), m_discovery(discovery) {
}

// ブロードキャストでホスト探索→JOIN送信
bool NetworkClient::discover_and_join(std::string& out_host_ip, int& currentChannel, bool verboseLogs) {
    for (int channelIdx = 0; channelIdx < NUM_CHANNELS; channelIdx++) {
        int discoveryPort = PORT_RANGES[channelIdx][1];
        uint8_t discover_pkt = PKT_DISCOVER;
        m_discovery.send_broadcast(discoveryPort, &discover_pkt, 1);
        if (verboseLogs) netlog_printf("CLIENT SENT DISCOVER broadcast port=%d (channel %d)", discoveryPort, channelIdx);

        char buf[MAX_UDP_PACKET];
        std::string from_ip;
        int from_port;

        auto start = std::chrono::steady_clock::now();
        // 1秒に短縮
        while (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count() < 1000) {
            int r = m_discovery.poll_recv(buf, sizeof(buf), from_ip, from_port, 200);
            if (r > 0) {
                uint8_t t = static_cast<uint8_t>(buf[0]);
                if (t == PKT_DISCOVER_REPLY) {
                    out_host_ip = from_ip;
                    m_hostIp = out_host_ip;
                    m_hostPort = PORT_RANGES[channelIdx][0];
                    currentChannel = channelIdx;
                    netlog_printf("CLIENT DISCOVERY: discovered host %s, set hostPort=%d channel=%d", m_hostIp.c_str(), m_hostPort, channelIdx);
                    uint8_t join_pkt = PKT_JOIN;
                    m_net.send_to(out_host_ip, m_hostPort, &join_pkt, 1);
                    return true;
                }
            }
        }
    }
    netlog_printf("CLIENT DISCOVERY: timeout, no host reply");
    return false;
}

// クライアント側パケット振り分け処理
void NetworkClient::process_packet(const char* buf, int len, const std::string& from_ip, int from_port,
                                    Game::GameObject* localPlayer, std::vector<std::shared_ptr<Game::GameObject>>& worldObjects) {
    if (len <= 0) return;
    uint8_t t = static_cast<uint8_t>(buf[0]);

    if (t == PKT_JOIN_ACK) {
        handle_join_ack(buf, len, from_ip, from_port);
    } else if (t == PKT_STATE) {
        PacketStateHeader hdr;
        const ObjectState* entries = nullptr;
        if (NetworkProtocol::parse_state_packet(buf, len, hdr, entries)) {
            handle_state(hdr, entries, localPlayer, worldObjects);
        }
    }
}

// JOIN_ACK処理：サーバーから割り当てられたプレイヤーIDを取得
void NetworkClient::handle_join_ack(const char* buf, int len, const std::string& from_ip, int from_port) {
    if (len >= 1 + 4) {
        uint32_t pid_net;
        memcpy(&pid_net, buf + 1, 4);
        m_myPlayerId = ntohl(pid_net);
        netlog_printf("CLIENT RECV JOIN_ACK id=%u from=%s:%d", m_myPlayerId, from_ip.c_str(), from_port);
    }
}

// STATE処理：ホストから受信した状態をワールドオブジェクトに適用
void NetworkClient::handle_state(const PacketStateHeader& hdr, const ObjectState* entries,
                                  Game::GameObject* localPlayer, std::vector<std::shared_ptr<Game::GameObject>>& worldObjects) {
    netlog_printf("CLIENT RECV STATE: objectCount=%u", hdr.objectCount);

    for (uint32_t i = 0; i < hdr.objectCount; ++i) {
        const ObjectState& os = entries[i];
        netlog_printf("CLIENT RECV STATE: Processing id=%u pos=(%.2f,%.2f,%.2f) rot=(%.2f,%.2f,%.2f)",
            os.id, os.posX, os.posY, os.posZ, os.rotX, os.rotY, os.rotZ);

        // 自分のプレイヤーIDの場合はスキップ（ローカル操作を優先）
        if (os.id == m_myPlayerId) {
            netlog_printf("CLIENT RECV STATE: Skipped self position sync for id=%u", os.id);
            continue;
        }

        bool applied = false;
        for (const auto& go : worldObjects) {
            if (go->getId() == os.id) {
                XMFLOAT3 oldPos = go->getPosition();
                go->setPosition({os.posX, os.posY, os.posZ});
                go->setRotation({os.rotX, os.rotY, os.rotZ});
                applied = true;
                netlog_printf("CLIENT RECV STATE: Updated existing worldObject id=%u from (%.2f,%.2f,%.2f) to (%.2f,%.2f,%.2f)",
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
            netlog_printf("CLIENT RECV STATE: Created new worldObject id=%u pos=(%.2f,%.2f,%.2f)",
                os.id, os.posX, os.posY, os.posZ);
        }
    }
}

// クライアント入力送信
void NetworkClient::send_input(const PacketInput& input) {
    netlog_printf("CLIENT_SEND_ATTEMPT seq=%u player=%u -> %s:%d time=%lld",
        input.seq, input.playerId, m_hostIp.c_str(), m_hostPort,
        (long long)std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count());
    if (m_hostIp.empty()) return;
    m_net.send_to(m_hostIp, m_hostPort, &input, static_cast<int>(sizeof(PacketInput)));
}

// クライアント側フレーム同期
void NetworkClient::frame_sync(Game::GameObject* localPlayer,
                                std::vector<std::shared_ptr<Game::GameObject>>& worldObjects) {
    if (!localPlayer) {
        netlog_printf("CLIENT FRAMESYNC: localPlayer is NULL");
        return;
    }

    netlog_printf("FRAMESYNC: Starting - isHost=0 myPlayerId=%u localPlayerID=%u",
        m_myPlayerId, localPlayer->getId());

    if (m_hostIp.empty()) {
        netlog_printf("CLIENT FRAMESYNC: hostIP is empty - SKIPPING");
        return;
    }
    // 重要: JOIN/ACK を受けてプレイヤーIDが割り当てられるまでSTATE送信を行わない
    if (m_myPlayerId == 0) {
        netlog_printf("CLIENT FRAMESYNC: myPlayerId is0 (not joined) - SKIPPING");
        return;
    }

    // ID=1とID=2の状態を構築
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
    auto states = build_states_for_ids(targetIds);
    auto buf = NetworkProtocol::build_state_packet(m_seq, states);

    bool sendSuccess = m_net.send_to(m_hostIp, m_hostPort, buf.data(), static_cast<int>(buf.size()));
    netlog_printf("CLIENT FRAMESYNC SEND: Sent %zu states to host %s:%d success=%d",
        states.size(), m_hostIp.c_str(), m_hostPort, sendSuccess ? 1 : 0);
}
