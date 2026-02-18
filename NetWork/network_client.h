#pragma once
// network_client.h - クライアント側のネットワーク処理

#include "udp_network.h"
#include "network_common.h"
#include "network_protocol.h"
#include <vector>
#include <string>
#include <memory>

namespace Game { class GameObject; }

// クライアント固有のネットワーク処理を担当するクラス
class NetworkClient {
public:
    NetworkClient(UdpNetwork& net, UdpNetwork& discovery);
    ~NetworkClient() = default;

    // ブロードキャストでホスト探索→JOIN送信（ブロック）
    bool discover_and_join(std::string& out_host_ip, int& currentChannel, bool verboseLogs);

    // パケット処理（クライアント側）
    void process_packet(const char* buf, int len, const std::string& from_ip, int from_port,
                        Game::GameObject* localPlayer, std::vector<std::shared_ptr<Game::GameObject>>& worldObjects);

    // 入力送信
    void send_input(const PacketInput& input);

    // フレーム同期（クライアント側）
    void frame_sync(Game::GameObject* localPlayer,
                    std::vector<std::shared_ptr<Game::GameObject>>& worldObjects);

    // アクセサ
    uint32_t getMyPlayerId() const { return m_myPlayerId; }
    const std::string& getHostIp() const { return m_hostIp; }
    int getHostPort() const { return m_hostPort; }

private:
    UdpNetwork& m_net;
    UdpNetwork& m_discovery;

    std::string m_hostIp;
    int m_hostPort = NET_PORT;
    uint32_t m_myPlayerId = 0;
    uint32_t m_seq = 0;

    // JOIN_ACK処理
    void handle_join_ack(const char* buf, int len, const std::string& from_ip, int from_port);

    // STATE処理
    void handle_state(const PacketStateHeader& hdr, const ObjectState* entries,
                      Game::GameObject* localPlayer, std::vector<std::shared_ptr<Game::GameObject>>& worldObjects);
};
