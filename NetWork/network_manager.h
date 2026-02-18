#pragma once
// network_manager.h - ホスト/クライアントの軽量化されたネットワーク管理

#include "udp_network.h"
#include "network_common.h"
#include <vector>
#include <unordered_map>
#include <memory>
#include <mutex>
#include <chrono>
#include <thread>
#include <condition_variable>
#include <deque>
#include <atomic>

namespace Game { class GameObject; } // 前方宣言（game_object.h をインクルードしてもOK）

class NetworkManager {
public:
    NetworkManager();
    ~NetworkManager();

    // 起動
    bool start_as_host();
    bool start_as_client();

    // client 用: ブロードキャストでホスト探索＆ JOIN まで行う（ブロック）
    bool discover_and_join(std::string& out_host_ip);

    // 毎フレーム呼出（受信処理はワーカースレッドが受け取り、ここではキューを処理する）
    void update(float dt, Game::GameObject* localPlayer, std::vector<Game::GameObject*>& worldObjects);

    // クライアント入力送信
    void send_input(const PacketInput& input);

    bool is_host() const { return m_isHost; }

    // accessor for client id
    uint32_t getMyPlayerId() const;

    // Frame-based sync called from main (e.g. every10 frames at60FPS) *** publicに移動 ***
    void FrameSync(Game::GameObject* localPlayer, std::vector<Game::GameObject*>& worldObjects);

    // チャンネル管理（既存機能）
    bool try_alternative_channels();
    void scan_channel_usage();
    int find_least_crowded_channel();
    bool switch_to_channel(int channelId);

    // 動的ポート管理（既存）
    bool try_dynamic_ports();

    // ファイアウォール例外（既存だが非強制）
    bool add_firewall_exception();

private:
    UdpNetwork m_net;        // 通常ポート (NET_PORT)
    UdpNetwork m_discovery;  // discovery 用ソケット
    bool m_isHost = false;

    std::mutex m_mutex;

    // ホスト側のみ
    struct ClientInfo {
        std::string ip;
        int port;
        uint32_t playerId;
        std::chrono::steady_clock::time_point lastSeen;
    };
    std::vector<ClientInfo> m_clients;
    uint32_t m_nextPlayerId = 1;
    uint32_t m_seq = 0;

    // クライアント側
    std::string m_hostIp;
    int m_hostPort = NET_PORT;
    uint32_t m_myPlayerId = 0;

    // チャンネル管理
    int m_currentChannel = 0;
    std::vector<ChannelInfo> m_channelInfo;
    std::chrono::steady_clock::time_point m_lastChannelScan;

    // 受信パケットキュー（ワーカが受け取り、update() で処理）
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

    // パフォーマンス/制限パラメータ
    size_t m_maxPacketsPerFrame = 8;            // 1フレームで処理する最大受信パケット数
    std::chrono::milliseconds m_stateInterval{200}; // ホストがSTATE送信する間隔（ワーカー側で分散送信）
    size_t m_stateSendIndex = 0;                // ローテーション送信インデックス

    // 内部ヘルパー（既存）
    void process_received(const char* buf, int len, const std::string& from_ip, int from_port, Game::GameObject* localPlayer, std::vector<Game::GameObject*>& worldObjects);
    void host_handle_join(const std::string& from_ip, int from_port, std::vector<Game::GameObject*>& worldObjects);
    void host_handle_input(const PacketInput& pi, std::vector<Game::GameObject*>& worldObjects);
    void client_handle_state(const PacketStateHeader& hdr, const ObjectState* entries, Game::GameObject* localPlayer, std::vector<Game::GameObject*>& worldObjects);
    void send_state_to_all(std::vector<Game::GameObject*>& worldObjects);

    // チャンネル機能
    void handle_channel_scan(const std::string& from_ip, int from_port);
    void handle_channel_info(const ChannelInfo& info);
    bool initialize_with_fallback();

    // ホストフリーズ防止用（ワーカー/メインで使う）
    void send_state_to_clients_round_robin(std::vector<Game::GameObject*>* worldObjectsPtr);

    // ワーカー起動/停止
    void start_worker();
    void stop_worker();

    // 受信キューへ追加（ワーカーから呼ぶ）
    void push_recv_packet(RecvPacket&& pkt);

    // ファイアウォール補助
    bool check_existing_firewall_rule();

    // ログ抑制フラグ（高頻度ログは無効）
    bool m_verboseLogs = false;
};

// グローバルインスタンス宣言
extern NetworkManager g_network;