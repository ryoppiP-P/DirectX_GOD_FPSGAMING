#pragma once
// network_manager.h - ホスト/クライアントの統括ネットワークマネージャー
// 実際の処理はNetworkHost/NetworkClientに委譲する

#include "udp_network.h"
#include "network_common.h"
#include "network_protocol.h"
#include "network_host.h"
#include "network_client.h"
#include <vector>
#include <memory>
#include <mutex>
#include <chrono>
#include <thread>
#include <condition_variable>
#include <deque>
#include <atomic>

namespace Game { class GameObject; }

class NetworkManager {
public:
    NetworkManager();
    ~NetworkManager();

    // 起動
    bool start_as_host();
    bool start_as_client();

    // client用: ブロードキャストでホスト探索→JOIN（ブロック）
    bool discover_and_join(std::string& out_host_ip);

    // 毎フレーム呼出（受信データはワーカースレッドが受け取り、ここではキューから処理する）
    void update(float dt, Game::GameObject* localPlayer, std::vector<std::shared_ptr<Game::GameObject>>& worldObjects);

    // クライアント入力送信
    void send_input(const PacketInput& input);

    bool is_host() const { return m_isHost; }

    // プレイヤーID取得
    uint32_t getMyPlayerId() const;

    // フレーム同期（メインから呼出、例: 60FPSで10フレーム毎 => 6Hz）
    void FrameSync(Game::GameObject* localPlayer, std::vector<std::shared_ptr<Game::GameObject>>& worldObjects);

    // チャンネル管理（補助機能）
    bool try_alternative_channels();
    void scan_channel_usage();
    int find_least_crowded_channel();
    bool switch_to_channel(int channelId);

    // 動的ポート管理
    bool try_dynamic_ports();

    // ファイアウォール例外（管理者権限非強制）
    bool add_firewall_exception();

private:
    UdpNetwork m_net;        // 通常ポート (NET_PORT)
    UdpNetwork m_discovery;  // discovery用ソケット
    bool m_isHost = false;

    std::mutex m_mutex;

    // ホスト/クライアント処理の委譲先
    std::unique_ptr<NetworkHost> m_host;
    std::unique_ptr<NetworkClient> m_client;

    // チャンネル管理
    int m_currentChannel = 0;
    std::vector<ChannelInfo> m_channelInfo;
    std::chrono::steady_clock::time_point m_lastChannelScan;

    // 受信パケットキュー（ワーカーが受け取り、update()で処理）
    struct RecvPacket {
        std::vector<char> data;
        int len;
        std::string from_ip;
        int from_port;
        bool isDiscovery;
    };
    std::deque<RecvPacket> m_recvQueue;
    std::mutex m_recvMutex;
    std::condition_variable m_recvCv;

    // ワーカースレッド
    std::thread m_worker;
    std::atomic<bool> m_workerRunning{false};

    // パフォーマンス/制御パラメータ
    size_t m_maxPacketsPerFrame = 8;
    std::chrono::milliseconds m_stateInterval{200};

    // パケット振り分け処理
    void process_received(const char* buf, int len, const std::string& from_ip, int from_port,
                          Game::GameObject* localPlayer, std::vector<std::shared_ptr<Game::GameObject>>& worldObjects);

    // チャンネル機能
    void handle_channel_scan(const std::string& from_ip, int from_port);
    void handle_channel_info(const ChannelInfo& info);
    bool initialize_with_fallback();

    // ワーカー起動/停止
    void start_worker();
    void stop_worker();

    // 受信キューへ追加（ワーカーから呼ぶ）
    void push_recv_packet(RecvPacket&& pkt);

    // ファイアウォール補助
    bool check_existing_firewall_rule();

    // ログ冗長フラグ（低頻度ログは無効）
    bool m_verboseLogs = false;
};

// グローバルインスタンス宣言
extern NetworkManager g_network;
