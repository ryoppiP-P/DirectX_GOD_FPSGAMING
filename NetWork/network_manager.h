#pragma once
// network_manager.h - �z�X�g/�N���C�A���g�̌y�ʉ����ꂽ�l�b�g���[�N�Ǘ�

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

namespace Game { class GameObject; } // �O���錾�igame_object.h ���C���N���[�h���Ă�OK�j

class NetworkManager {
public:
    NetworkManager();
    ~NetworkManager();

    // �N��
    bool start_as_host();
    bool start_as_client();

    // client �p: �u���[�h�L���X�g�Ńz�X�g�T���� JOIN �܂ōs���i�u���b�N�j
    bool discover_and_join(std::string& out_host_ip);

    // ���t���[���ďo�i��M�����̓��[�J�[�X���b�h���󂯎��A�����ł̓L���[����������j
    void update(float dt, Game::GameObject* localPlayer, std::vector<std::shared_ptr<Game::GameObject>>& worldObjects);

    // �N���C�A���g���͑��M
    void send_input(const PacketInput& input);

    bool is_host() const { return m_isHost; }

    // accessor for client id
    uint32_t getMyPlayerId() const;

    // Frame-based sync called from main (e.g. every10 frames at60FPS) *** public�Ɉړ� ***
    void FrameSync(Game::GameObject* localPlayer, std::vector<std::shared_ptr<Game::GameObject>>& worldObjects);

    // �`�����l���Ǘ��i�����@�\�j
    bool try_alternative_channels();
    void scan_channel_usage();
    int find_least_crowded_channel();
    bool switch_to_channel(int channelId);

    // ���I�|�[�g�Ǘ��i�����j
    bool try_dynamic_ports();

    // �t�@�C�A�E�H�[����O�i���������񋭐��j
    bool add_firewall_exception();

private:
    UdpNetwork m_net;        // �ʏ�|�[�g (NET_PORT)
    UdpNetwork m_discovery;  // discovery �p�\�P�b�g
    bool m_isHost = false;

    std::mutex m_mutex;

    // �z�X�g���̂�
    struct ClientInfo {
        std::string ip;
        int port;
        uint32_t playerId;
        std::chrono::steady_clock::time_point lastSeen;
    };
    std::vector<ClientInfo> m_clients;
    uint32_t m_nextPlayerId = 1;
    uint32_t m_seq = 0;

    // �N���C�A���g��
    std::string m_hostIp;
    int m_hostPort = NET_PORT;
    uint32_t m_myPlayerId = 0;

    // �`�����l���Ǘ�
    int m_currentChannel = 0;
    std::vector<ChannelInfo> m_channelInfo;
    std::chrono::steady_clock::time_point m_lastChannelScan;

    // ��M�p�P�b�g�L���[�i���[�J���󂯎��Aupdate() �ŏ����j
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

    // ���[�J�[�X���b�h
    std::thread m_worker;
    std::atomic<bool> m_workerRunning{false};

    // �p�t�H�[�}���X/�����p�����[�^
    size_t m_maxPacketsPerFrame = 8;            // 1�t���[���ŏ�������ő��M�p�P�b�g��
    std::chrono::milliseconds m_stateInterval{200}; // �z�X�g��STATE���M����Ԋu�i���[�J�[���ŕ��U���M�j
    size_t m_stateSendIndex = 0;                // ���[�e�[�V�������M�C���f�b�N�X

    // �����w���p�[�i�����j
    void process_received(const char* buf, int len, const std::string& from_ip, int from_port, Game::GameObject* localPlayer, std::vector<std::shared_ptr<Game::GameObject>>& worldObjects);
    void host_handle_join(const std::string& from_ip, int from_port, std::vector<std::shared_ptr<Game::GameObject>>& worldObjects);
    void host_handle_input(const PacketInput& pi, std::vector<std::shared_ptr<Game::GameObject>>& worldObjects);
    void client_handle_state(const PacketStateHeader& hdr, const ObjectState* entries, Game::GameObject* localPlayer, std::vector<std::shared_ptr<Game::GameObject>>& worldObjects);
    void send_state_to_all(std::vector<std::shared_ptr<Game::GameObject>>& worldObjects);

    // �`�����l���@�\
    void handle_channel_scan(const std::string& from_ip, int from_port);
    void handle_channel_info(const ChannelInfo& info);
    bool initialize_with_fallback();

    // �z�X�g�t���[�Y�h�~�p�i���[�J�[/���C���Ŏg���j
    void send_state_to_clients_round_robin(std::vector<std::shared_ptr<Game::GameObject>>* worldObjectsPtr);

    // ���[�J�[�N��/��~
    void start_worker();
    void stop_worker();

    // ��M�L���[�֒ǉ��i���[�J�[����Ăԁj
    void push_recv_packet(RecvPacket&& pkt);

    // �t�@�C�A�E�H�[���⏕
    bool check_existing_firewall_rule();

    // ���O�}���t���O�i���p�x���O�͖����j
    bool m_verboseLogs = false;
};

// �O���[�o���C���X�^���X�錾
extern NetworkManager g_network;