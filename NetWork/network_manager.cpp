// network_manager.cpp - ネットワークマネージャー（統括のみ）
// 実際のホスト/クライアント処理はNetworkHost/NetworkClientに委譲
#include "pch.h"
#include "network_manager.h"
#include "Game/Objects/game_object.h"
#include <iostream>
#include <cstring>
#include <chrono>
#include <algorithm>

NetworkManager g_network; // グローバルインスタンス定義

NetworkManager::NetworkManager() {
    m_lastChannelScan = std::chrono::steady_clock::now();
}

NetworkManager::~NetworkManager() {
    stop_worker();
    m_net.close_socket();
    m_discovery.close_socket();
}

uint32_t NetworkManager::getMyPlayerId() const {
    if (m_client) return m_client->getMyPlayerId();
    if (m_isHost) return 1;
    return 0;
}

// ===== 起動処理 =====

bool NetworkManager::start_as_host() {
    add_firewall_exception();
    if (!initialize_with_fallback()) {
        netlog_printf("HOST INITIALIZATION FAILED");
        return false;
    }
    m_isHost = true;
    m_host = std::make_unique<NetworkHost>(m_net, m_mutex);
    start_worker();
    netlog_printf("STARTED AS HOST, net port=%d discovery port=%d", m_net.get_current_port(), m_discovery.get_current_port());
    return true;
}

bool NetworkManager::start_as_client() {
    add_firewall_exception();
    if (!m_net.initialize_dynamic_port()) {
        if (!m_net.initialize(0)) {
            netlog_printf("CLIENT INITIALIZATION FAILED");
            return false;
        }
    }
    if (!m_discovery.initialize_broadcast(DISCOVERY_PORT + 1)) {
        netlog_printf("CLIENT DISCOVERY INITIALIZATION FAILED");
        return false;
    }
    m_isHost = false;
    m_client = std::make_unique<NetworkClient>(m_net, m_discovery);
    start_worker();
    netlog_printf("STARTED AS CLIENT, auto-assigned port=%d, discovery port=%d", m_net.get_current_port(), DISCOVERY_PORT + 1);
    return true;
}

// ===== ディスカバリ =====

bool NetworkManager::discover_and_join(std::string& out_host_ip) {
    scan_channel_usage();
    if (m_client) {
        return m_client->discover_and_join(out_host_ip, m_currentChannel, m_verboseLogs);
    }
    return false;
}

// ===== メインループ更新 =====

void NetworkManager::update(float dt, Game::GameObject* localPlayer, std::vector<std::shared_ptr<Game::GameObject>>& worldObjects) {
    size_t processed = 0;
    while (processed < m_maxPacketsPerFrame) {
        RecvPacket pkt;
        {
            std::lock_guard<std::mutex> lk(m_recvMutex);
            if (m_recvQueue.empty()) break;
            pkt = std::move(m_recvQueue.front());
            m_recvQueue.pop_front();
        }
        process_received(pkt.data.data(), pkt.len, pkt.from_ip, pkt.from_port, localPlayer, worldObjects);
        ++processed;
    }
}

// パケット振り分け：ホスト/クライアントに委譲
void NetworkManager::process_received(const char* buf, int len, const std::string& from_ip, int from_port,
                                       Game::GameObject* localPlayer, std::vector<std::shared_ptr<Game::GameObject>>& worldObjects) {
    if (len <= 0) return;
    if (m_isHost && m_host) {
        m_host->process_packet(buf, len, from_ip, from_port, localPlayer, worldObjects);
    } else if (!m_isHost && m_client) {
        m_client->process_packet(buf, len, from_ip, from_port, localPlayer, worldObjects);
    }
}

// ===== 入力送信 =====

void NetworkManager::send_input(const PacketInput& input) {
    if (m_isHost) return;
    if (m_client) {
        m_client->send_input(input);
    }
}

// ===== フレーム同期 =====

void NetworkManager::FrameSync(Game::GameObject* localPlayer, std::vector<std::shared_ptr<Game::GameObject>>& worldObjects) {
    if (m_isHost && m_host) {
        m_host->frame_sync(localPlayer, worldObjects);
    } else if (!m_isHost && m_client) {
        m_client->frame_sync(localPlayer, worldObjects);
    }
}

// ===== チャンネル管理 =====

void NetworkManager::scan_channel_usage() {
    auto now = std::chrono::steady_clock::now();
    if (std::chrono::duration_cast<std::chrono::seconds>(now - m_lastChannelScan).count() < 30) {
        return;
    }
    m_lastChannelScan = now;
    netlog_printf("CHANNEL SCAN: skipped for performance");
}

int NetworkManager::find_least_crowded_channel() {
    if (m_channelInfo.empty()) return 0;
    int bestChannel = 0;
    uint32_t minUsers = UINT32_MAX;
    for (const auto& info : m_channelInfo) {
        if (info.userCount < minUsers) {
            minUsers = info.userCount;
            bestChannel = static_cast<int>(info.channelId);
        }
    }
    return bestChannel;
}

bool NetworkManager::switch_to_channel(int channelId) {
    if (channelId < 0 || channelId >= NUM_CHANNELS) return false;
    m_net.close_socket();
    m_discovery.close_socket();
    int newNetPort = PORT_RANGES[channelId][0];
    int newDiscoveryPort = PORT_RANGES[channelId][1];
    if (!m_net.initialize(newNetPort) || !m_discovery.initialize_broadcast(newDiscoveryPort)) {
        netlog_printf("CHANNEL SWITCH FAILED: channel %d", channelId);
        return false;
    }
    m_currentChannel = channelId;
    netlog_printf("CHANNEL SWITCHED: to channel %d (net=%d, discovery=%d)", channelId, newNetPort, newDiscoveryPort);
    return true;
}

bool NetworkManager::try_alternative_channels() {
    for (int i = 1; i < NUM_CHANNELS; i++) {
        if (switch_to_channel(i)) return true;
    }
    return false;
}

bool NetworkManager::try_dynamic_ports() {
    return m_net.initialize_dynamic_port() && m_discovery.initialize_dynamic_port();
}

// ===== ファイアウォール =====

bool NetworkManager::check_existing_firewall_rule() {
    return false;
}

bool NetworkManager::add_firewall_exception() {
    BOOL isAdmin = FALSE;
    PSID administratorsGroup = NULL;
    SID_IDENTIFIER_AUTHORITY ntAuthority = SECURITY_NT_AUTHORITY;
    if (AllocateAndInitializeSid(&ntAuthority, 2, SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS,
        0, 0, 0, 0, 0, 0, &administratorsGroup)) {
        CheckTokenMembership(NULL, administratorsGroup, &isAdmin);
        FreeSid(administratorsGroup);
    }
    if (!isAdmin) {
        netlog_printf("FIREWALL: not admin, will use dynamic ports if needed");
        return true;
    }
    netlog_printf("FIREWALL: admin present, but skipping explicit rule add for performance");
    return true;
}

// ===== チャンネルハンドラ =====

void NetworkManager::handle_channel_scan(const std::string& from_ip, int from_port) {
    ChannelInfo info;
    info.type = PKT_CHANNEL_INFO;
    info.channelId = static_cast<uint32_t>(m_currentChannel);
    info.userCount = m_host ? static_cast<uint32_t>(m_host->clients().size()) : 0;
    info.basePort = static_cast<uint32_t>(m_net.get_current_port());
    info.discoveryPort = static_cast<uint32_t>(m_discovery.get_current_port());
    m_discovery.send_to(from_ip, from_port, &info, sizeof(info));
}

void NetworkManager::handle_channel_info(const ChannelInfo& info) {
    bool found = false;
    for (auto& existing : m_channelInfo) {
        if (existing.channelId == info.channelId) {
            existing = info;
            found = true;
            break;
        }
    }
    if (!found) m_channelInfo.push_back(info);
}

// ===== ワーカースレッド =====

void NetworkManager::start_worker() {
    if (m_workerRunning.exchange(true)) return;
    m_worker = std::thread([this]() {
        auto lastStateSend = std::chrono::steady_clock::now();
        while (m_workerRunning.load()) {
            char buf[MAX_UDP_PACKET];
            std::string from_ip;
            int from_port = 0;

            // メインネットソケットのポーリング
            int r = m_net.poll_recv(buf, sizeof(buf), from_ip, from_port, 50);
            if (r > 0) {
                RecvPacket pkt;
                pkt.data.assign(buf, buf + r);
                pkt.len = r;
                pkt.from_ip = from_ip;
                pkt.from_port = from_port;
                pkt.isDiscovery = false;
                push_recv_packet(std::move(pkt));
            }

            // ディスカバリソケットのポーリング
            int dr = m_discovery.poll_recv(buf, sizeof(buf), from_ip, from_port, 10);
            if (dr > 0) {
                uint8_t dt = static_cast<uint8_t>(buf[0]);
                if (m_isHost && dt == PKT_DISCOVER) {
                    // 即座にリプライ（軽量処理）
                    uint8_t reply = PKT_DISCOVER_REPLY;
                    m_discovery.send_to(from_ip, from_port, &reply, 1);
                } else {
                    RecvPacket pkt;
                    pkt.data.assign(buf, buf + dr);
                    pkt.len = dr;
                    pkt.from_ip = from_ip;
                    pkt.from_port = from_port;
                    pkt.isDiscovery = true;
                    push_recv_packet(std::move(pkt));
                }
            }

            // 定期的なSTATE送信（ホスト時のみ、ラウンドロビン方式）
            auto now = std::chrono::steady_clock::now();
            if (m_isHost && m_host &&
                std::chrono::duration_cast<std::chrono::milliseconds>(now - lastStateSend) >= m_stateInterval) {
                // ワーカーからは最小限のSTATEを送信（ネットワーク維持目的）
                std::lock_guard<std::mutex> lk(m_mutex);
                if (m_host->has_clients()) {
                    PacketStateHeader header;
                    header.type = PKT_STATE;
                    header.seq = m_host->next_seq();
                    header.objectCount = 1;
                    ObjectState os = {};
                    const auto& clients = m_host->clients();
                    if (!clients.empty()) {
                        os.id = clients[0].playerId;
                    }
                    os.posX = os.posY = os.posZ = 0.0f;
                    os.rotX = os.rotY = os.rotZ = 0.0f;

                    std::vector<char> sendbuf;
                    sendbuf.resize(sizeof(header) + sizeof(os));
                    memcpy(sendbuf.data(), &header, sizeof(header));
                    memcpy(sendbuf.data() + sizeof(header), &os, sizeof(os));
                    if (!clients.empty()) {
                        m_net.send_to(clients[0].ip, clients[0].port, sendbuf.data(), static_cast<int>(sendbuf.size()));
                    }
                }
                lastStateSend = now;
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
    });
}

void NetworkManager::stop_worker() {
    if (!m_workerRunning.exchange(false)) return;
    if (m_worker.joinable()) {
        m_worker.join();
    }
}

// 受信キューへ追加（容量制限でメモリ溢れ防止）
void NetworkManager::push_recv_packet(RecvPacket&& pkt) {
    std::lock_guard<std::mutex> lk(m_recvMutex);
    const size_t MAX_QUEUE = 1024;
    if (m_recvQueue.size() >= MAX_QUEUE) {
        m_recvQueue.pop_front();
    }
    m_recvQueue.push_back(std::move(pkt));
}

// フォールバック初期化
bool NetworkManager::initialize_with_fallback() {
    if (m_net.initialize(NET_PORT) && m_discovery.initialize_broadcast(DISCOVERY_PORT)) {
        return true;
    }
    for (int i = 1; i < NUM_CHANNELS; i++) {
        int netPort = PORT_RANGES[i][0];
        int discoveryPort = PORT_RANGES[i][1];
        if (m_net.initialize(netPort) && m_discovery.initialize_broadcast(discoveryPort)) {
            m_currentChannel = i;
            return true;
        }
    }
    if (try_dynamic_ports()) return true;
    return false;
}
