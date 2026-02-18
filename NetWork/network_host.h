#pragma once
// network_host.h - ホスト/サーバー側のネットワーク処理

#include "udp_network.h"
#include "network_common.h"
#include "network_protocol.h"
#include <vector>
#include <string>
#include <memory>
#include <mutex>
#include <chrono>

namespace Game { class GameObject; }

// ホスト固有のネットワーク処理を担当するクラス
class NetworkHost {
public:
    // 接続クライアント情報
    struct ClientInfo {
        std::string ip;
        int port;
        uint32_t playerId;
        std::chrono::steady_clock::time_point lastSeen;
    };

    NetworkHost(UdpNetwork& net, std::mutex& sharedMutex);
    ~NetworkHost() = default;

    // パケット処理
    void process_packet(const char* buf, int len, const std::string& from_ip, int from_port,
                        Game::GameObject* localPlayer, std::vector<std::shared_ptr<Game::GameObject>>& worldObjects);

    // JOIN処理
    void handle_join(const std::string& from_ip, int from_port,
                     std::vector<std::shared_ptr<Game::GameObject>>& worldObjects);

    // INPUT処理
    void handle_input(const PacketInput& pi,
                      std::vector<std::shared_ptr<Game::GameObject>>& worldObjects);

    // クライアントから受信したSTATE処理
    void handle_client_state(const char* buf, int len, const std::string& from_ip, int from_port,
                             std::vector<std::shared_ptr<Game::GameObject>>& worldObjects);

    // 全クライアントへSTATE送信
    void send_state_to_all(std::vector<std::shared_ptr<Game::GameObject>>& worldObjects);

    // ラウンドロビン方式でSTATE送信（ワーカースレッドから呼ぶ）
    void send_state_round_robin(std::vector<std::shared_ptr<Game::GameObject>>* worldObjectsPtr);

    // フレーム同期（ホスト側）
    void frame_sync(Game::GameObject* localPlayer,
                    std::vector<std::shared_ptr<Game::GameObject>>& worldObjects);

    // クライアント一覧アクセス
    const std::vector<ClientInfo>& clients() const { return m_clients; }
    bool has_clients() const;

    // シーケンス番号参照（ワーカースレッドでの最小STATE送信用）
    uint32_t next_seq() { return m_seq++; }

private:
    UdpNetwork& m_net;
    std::mutex& m_mutex;

    std::vector<ClientInfo> m_clients;
    uint32_t m_nextPlayerId = 2;
    uint32_t m_seq = 0;
    size_t m_stateSendIndex = 0;
};
