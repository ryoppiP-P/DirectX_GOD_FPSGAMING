// network_manager.cpp
#include "network_manager.h"
#include "Game/Objects/game_object.h"
#include "Game/Objects/camera.h"
#include "main.h"
#include "Engine/Graphics/primitive.h"
#include <iostream>
#include <cstring>
#include <chrono>
#include <fstream>
#include <mutex>
#include <cstdarg>
#include <algorithm>
#include <condition_variable>

NetworkManager g_network; // グローバルインスタンス定義
static auto lastQueueLog = std::chrono::steady_clock::now();
auto now = std::chrono::steady_clock::now();

// Simple thread-safe network logger (��p�x�̂݃��O�o��)
static std::mutex g_netlog_mutex;
static void netlog_printf(const char* fmt, ...) {
    std::lock_guard<std::mutex> lk(g_netlog_mutex);
    char buf[1024];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);

    // timestamp (�Z��)
    auto now = std::chrono::system_clock::now();
    std::time_t t = std::chrono::system_clock::to_time_t(now);
    char timebuf[32];
    tm local_tm;
#ifdef _WIN32
    localtime_s(&local_tm, &t);
    strftime(timebuf, sizeof(timebuf), "%H:%M:%S", &local_tm);
#else
    localtime_r(&t, &local_tm);
    strftime(timebuf, sizeof(timebuf), "%H:%M:%S", &local_tm);
#endif

    std::ofstream ofs("dx_netlog.txt", std::ios::app);
    if (ofs) {
        ofs << "[" << timebuf << "] " << buf << std::endl;
    }
}

NetworkManager::NetworkManager() {
    m_lastChannelScan = std::chrono::steady_clock::now();
}

NetworkManager::~NetworkManager() {
    stop_worker();
    m_net.close_socket();
    m_discovery.close_socket();
}

uint32_t NetworkManager::getMyPlayerId() const {
    return m_myPlayerId;
}

// start_as_host/start_as_client �̓\�P�b�g��������Ƀ��[�J�[���N������
bool NetworkManager::start_as_host() {
    add_firewall_exception();
    if (!initialize_with_fallback()) {
        netlog_printf("HOST INITIALIZATION FAILED");
        return false;
    }
    m_isHost = true;
    // Reserve player ID1 for the host's local player; start assigning clients at2
    m_nextPlayerId =2;
    // ���[�J�[�J�n
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
    start_worker();
    netlog_printf("STARTED AS CLIENT, auto-assigned port=%d, discovery port=%d", m_net.get_current_port(), DISCOVERY_PORT + 1);
    return true;
}

// discover_and_join �̓���͕ς����i�u���b�N�^�T���j�����A�T���p�̑҂����Ԃ�Z�k
bool NetworkManager::discover_and_join(std::string& out_host_ip) {
    scan_channel_usage();
    for (int channelIdx = 0; channelIdx < NUM_CHANNELS; channelIdx++) {
        int discoveryPort = PORT_RANGES[channelIdx][1];
        uint8_t discover_pkt = PKT_DISCOVER;
        m_discovery.send_broadcast(discoveryPort, &discover_pkt, 1);
        if (m_verboseLogs) netlog_printf("CLIENT SENT DISCOVER broadcast port=%d (channel %d)", discoveryPort, channelIdx);

        char buf[MAX_UDP_PACKET];
        std::string from_ip;
        int from_port;

        auto start = std::chrono::steady_clock::now();
        // 1 �b�ɒZ�k
        while (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count() < 1000) {
            int r = m_discovery.poll_recv(buf, sizeof(buf), from_ip, from_port, 200);
            if (r > 0) {
                uint8_t t = (uint8_t)buf[0];
                if (t == PKT_DISCOVER_REPLY) {
                    out_host_ip = from_ip;
                    m_hostIp = out_host_ip;
                    m_hostPort = PORT_RANGES[channelIdx][0];
                    m_currentChannel = channelIdx;
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

// update: ���[�J�[���󂯎��L���[�ɐς񂾃p�P�b�g���ő� m_maxPacketsPerFrame ����������i���Ɍy�ʁj
void NetworkManager::update(float dt, GameObject* localPlayer, std::vector<GameObject*>& worldObjects) {
    // process up to N packets from the recv queue
    size_t processed = 0;
    while (processed < m_maxPacketsPerFrame) {
        RecvPacket pkt;
        {
            std::lock_guard<std::mutex> lk(m_recvMutex);
            if (m_recvQueue.empty()) break;
            pkt = std::move(m_recvQueue.front());
            m_recvQueue.pop_front();
        }
        // discovery packets handled by worker when host (reply sent immediately).
        // Only process game-level packets here.
        process_received(pkt.data.data(), pkt.len, pkt.from_ip, pkt.from_port, localPlayer, worldObjects);
        ++processed;
    }
}

// process_received remains mostly the same, called from main thread for game logic packets.
void NetworkManager::process_received(const char* buf, int len, const std::string& from_ip, int from_port, GameObject* localPlayer, std::vector<GameObject*>& worldObjects) {
    if (len <= 0) return;
    uint8_t t = (uint8_t)buf[0];
    if (m_isHost) {
        if (t == PKT_JOIN) {
            netlog_printf("HOST RECV JOIN from=%s:%d bytes=%d", from_ip.c_str(), from_port, len);
            host_handle_join(from_ip, from_port, worldObjects);
        } else if (t == PKT_INPUT) {
            if (len >= (int)sizeof(PacketInput)) {
                PacketInput pi;
                memcpy(&pi, buf, sizeof(PacketInput));
                host_handle_input(pi, worldObjects);

                // �ŏI�ڐG�����X�V
                std::lock_guard<std::mutex> lk(m_mutex);
                for (auto& client : m_clients) {
                    if (client.playerId == pi.playerId) {
                        client.lastSeen = std::chrono::steady_clock::now();
                        break;
                    }
                }
            }
        } else if (t == PKT_PING) {
            // optional lightweight handling
        } else if (t == PKT_STATE) {
            // Client sent its state to host (host should apply/update worldObjects)
            if (len >= (int)sizeof(PacketStateHeader)) {
                PacketStateHeader hdr;
                memcpy(&hdr, buf, sizeof(hdr));
                const char* p = buf + sizeof(hdr);
                size_t expected = sizeof(hdr) + (size_t)hdr.objectCount * sizeof(ObjectState);
                if (len >= (int)expected) {
                    const ObjectState* entries = (const ObjectState*)p;
                    netlog_printf("HOST RECV STATE from %s:%d objectCount=%u", from_ip.c_str(), from_port, hdr.objectCount);

                    for (uint32_t i = 0; i < hdr.objectCount; ++i) {
                        const ObjectState& os = entries[i];
                        netlog_printf("HOST RECV STATE: Processing client id=%u pos=(%.2f,%.2f,%.2f)",
                            os.id, os.posX, os.posY, os.posZ);

                        bool applied = false;
                        for (auto* go : worldObjects) {
                            if (go->getId() == os.id) {
                                XMFLOAT3 oldPos = go->getPosition();
                                go->setPosition({ os.posX, os.posY, os.posZ });
                                go->setRotation({ os.rotX, os.rotY, os.rotZ });
                                applied = true;
                                netlog_printf("HOST RECV STATE: Updated client id=%u from (%.2f,%.2f,%.2f) to (%.2f,%.2f,%.2f)",
                                    os.id, oldPos.x, oldPos.y, oldPos.z, os.posX, os.posY, os.posZ);
                                break;
                            }
                        }
                        if (!applied) {
                            GameObject* newObj = new GameObject();
                            newObj->setId(os.id);
                            newObj->setPosition({ os.posX, os.posY, os.posZ });
                            newObj->setRotation({ os.rotX, os.rotY, os.rotZ });
                            newObj->setMesh(Box, 36, GetPolygonTexture());
                            newObj->setBoxCollider({ 0.8f, 1.8f, 0.8f });
                            worldObjects.push_back(newObj);
                            netlog_printf("HOST RECV STATE: Created new client object id=%u pos=(%.2f,%.2f,%.2f)",
                                os.id, os.posX, os.posY, os.posZ);
                        }
                        // update client's last seen timestamp
                        std::lock_guard<std::mutex> lk(m_mutex);
                        for (auto& client : m_clients) {
                            if (client.playerId == os.id) {
                                client.lastSeen = std::chrono::steady_clock::now();
                                break;
                            }
                        }
                    }
                }
            }
        }
    } else {
        if (t == PKT_JOIN_ACK) {
            if (len >= 1 + 4) {
                uint32_t pid_net;
                memcpy(&pid_net, buf + 1, 4);
                m_myPlayerId = ntohl(pid_net);
                netlog_printf("CLIENT RECV JOIN_ACK id=%u from=%s:%d", m_myPlayerId, from_ip.c_str(), from_port);
            }
        } else if (t == PKT_STATE) {
            if (len >= (int)sizeof(PacketStateHeader)) {
                PacketStateHeader hdr;
                memcpy(&hdr, buf, sizeof(hdr));
                const char* p = buf + sizeof(hdr);
                size_t expected = sizeof(hdr) + (size_t)hdr.objectCount * sizeof(ObjectState);
                if (len >= (int)expected) {
                    client_handle_state(hdr, (const ObjectState*)p, localPlayer, worldObjects);
                }
            }
        }
    }
}

void NetworkManager::host_handle_join(const std::string& from_ip, int from_port, std::vector<GameObject*>& worldObjects) {
    //�����N���C�A���g�`�F�b�N
    {
        std::lock_guard<std::mutex> lk(m_mutex);
        for (auto& c : m_clients) {
            if (c.ip == from_ip && c.port == from_port) {
                netlog_printf("HOST JOIN: Client %s:%d already exists - ignoring", from_ip.c_str(), from_port);
                return;
            }
        }
    }

    // Determine a non-conflicting player ID (never assign1 to remote clients)
    uint32_t assignedId =0;
    {
        std::lock_guard<std::mutex> lk(m_mutex);
        uint32_t cand = m_nextPlayerId;
        if (cand <=1) cand =2;
        // find next unused id (check existing clients and worldObjects)
        bool conflict = true;
        while (conflict) {
            conflict = false;
            for (const auto& c : m_clients) {
                if (c.playerId == cand) { conflict = true; break; }
            }
            if (conflict) { ++cand; continue; }
            for (auto* go : worldObjects) {
                if (go && go->getId() == cand) { conflict = true; break; }
            }
            if (conflict) ++cand;
        }
        assignedId = cand;
        // register client
        ClientInfo ci;
        ci.ip = from_ip;
        ci.port = from_port;
        ci.playerId = assignedId;
        ci.lastSeen = std::chrono::steady_clock::now();
        m_clients.push_back(ci);
        // advance next id
        m_nextPlayerId = assignedId +1;
    }

    netlog_printf("HOST NEW CLIENT: %s:%d assigned id=%u (total clients: %zu)", from_ip.c_str(), from_port, assignedId, m_clients.size());

    // If a GameObject with this id already exists (pre-created players), reuse it. Otherwise create a new one.
    bool existed = false;
    for (auto* go : worldObjects) {
        if (go && go->getId() == assignedId) {
            existed = true;
            break;
        }
    }

    if (!existed) {
        GameObject* newObj = new GameObject();
        newObj->setPosition({0.0f,3.0f,0.0f });
        newObj->setRotation({0.0f,0.0f,0.0f });
        newObj->setId(assignedId);
        newObj->setMesh(Box,36, GetPolygonTexture());
        newObj->setBoxCollider({0.8f,1.8f,0.8f });

        worldObjects.push_back(newObj);
        netlog_printf("HOST: Created worldObject for client id=%u", assignedId);
    } else {
        netlog_printf("HOST: Reusing existing worldObject for client id=%u", assignedId);
    }

    // send JOIN_ACK once
    uint8_t reply[1 +4];
    reply[0] = PKT_JOIN_ACK;
    uint32_t pid_net = htonl(assignedId);
    memcpy(reply +1, &pid_net,4);
    bool sendSuccess = m_net.send_to(from_ip, from_port, reply, (int)(1 +4));
    netlog_printf("HOST SEND JOIN_ACK -> %s:%d id=%u success=%d", from_ip.c_str(), from_port, assignedId, sendSuccess ?1 :0);
}

void NetworkManager::host_handle_input(const PacketInput& pi, std::vector<GameObject*>& worldObjects) {
    for (auto* go : worldObjects) {
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

void NetworkManager::client_handle_state(const PacketStateHeader& hdr, const ObjectState* entries, GameObject* localPlayer, std::vector<GameObject*>& worldObjects) {
    netlog_printf("CLIENT RECV STATE: objectCount=%u", hdr.objectCount);

    for (uint32_t i = 0; i < hdr.objectCount; ++i) {
        const ObjectState& os = entries[i];
        netlog_printf("CLIENT RECV STATE: Processing id=%u pos=(%.2f,%.2f,%.2f) rot=(%.2f,%.2f,%.2f)",
            os.id, os.posX, os.posY, os.posZ, os.rotX, os.rotY, os.rotZ);

        // �����̃v���C���[ID�̏ꍇ�̓X�L�b�v�i���[�J������D��j
        if (os.id == m_myPlayerId) {
            netlog_printf("CLIENT RECV STATE: Skipped self position sync for id=%u", os.id);
            continue;
        }

        bool applied = false;
        for (auto* go : worldObjects) {
            if (go->getId() == os.id) {
                XMFLOAT3 oldPos = go->getPosition();
                go->setPosition({ os.posX, os.posY, os.posZ });
                go->setRotation({ os.rotX, os.rotY, os.rotZ });
                applied = true;
                netlog_printf("CLIENT RECV STATE: Updated existing worldObject id=%u from (%.2f,%.2f,%.2f) to (%.2f,%.2f,%.2f)",
                    os.id, oldPos.x, oldPos.y, oldPos.z, os.posX, os.posY, os.posZ);
                break;
            }
        }
        if (!applied) {
            GameObject* newObj = new GameObject();
            newObj->setId(os.id);
            newObj->setPosition({ os.posX, os.posY, os.posZ });
            newObj->setRotation({ os.rotX, os.rotY, os.rotZ });
            newObj->setMesh(Box, 36, GetPolygonTexture());
            newObj->setBoxCollider({ 0.8f, 1.8f, 0.8f });
            worldObjects.push_back(newObj);
            netlog_printf("CLIENT RECV STATE: Created new worldObject id=%u pos=(%.2f,%.2f,%.2f)",
                os.id, os.posX, os.posY, os.posZ);
        }
    }
}

void NetworkManager::send_input(const PacketInput& input) {
    netlog_printf("CLIENT_SEND_ATTEMPT seq=%u player=%u -> %s:%d time=%lld",
        input.seq, input.playerId, m_hostIp.c_str(), m_hostPort,
        (long long)std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count());
    if (m_isHost) return;
    if (m_hostIp.empty()) return;
    m_net.send_to(m_hostIp, m_hostPort, &input, (int)sizeof(PacketInput));
}

// send_state_to_clients_round_robin: 1�N���C�A���g�������𑗂��ĕ��ׂ𕪎U�i���[�J�[����Ăԁj
void NetworkManager::send_state_to_clients_round_robin(std::vector<GameObject*>* worldObjectsPtr) {
    std::lock_guard<std::mutex> lk(m_mutex);
    if (m_clients.empty() || worldObjectsPtr == nullptr) return;

    // ���M�Ώۂ͈�l���i���[�e�[�V�����j
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
    for (auto* go : *worldObjectsPtr) {
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

// FrameSync: called from main every N frames (we'll call every10 frames at60FPS =>6Hz)
void NetworkManager::FrameSync(GameObject* localPlayer, std::vector<GameObject*>& worldObjects) {
    if (!localPlayer) {
        netlog_printf("FRAMESYNC: localPlayer is NULL");
        return;
    }

    netlog_printf("FRAMESYNC: Starting - isHost=%d myPlayerId=%u localPlayerID=%u",
        m_isHost ? 1 : 0, m_myPlayerId, localPlayer->getId());

    // We always send/receive positions for player IDs 1 and 2
    auto build_states_for_ids = [&](const std::vector<int>& ids) {
        std::vector<ObjectState> states;
        for (int id : ids) {
            ObjectState os = {};
            os.id = static_cast<uint32_t>(id);
            bool found = false;
            // check localPlayer first
            if (localPlayer && localPlayer->getId() == (uint32_t)id) {
                auto p = localPlayer->getPosition();
                auto r = localPlayer->getRotation();
                os.posX = p.x; os.posY = p.y; os.posZ = p.z;
                os.rotX = r.x; os.rotY = r.y; os.rotZ = r.z;
                found = true;
            } else {
                for (auto* go : worldObjects) {
                    if (!go) continue;
                    if (go->getId() == (uint32_t)id) {
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

    std::vector<int> targetIds = { 1, 2 };

    if (m_isHost) {
        // Host: send its local players (ids 1 and 2) to all clients
        std::lock_guard<std::mutex> lk(m_mutex);
        if (m_clients.empty()) {
            netlog_printf("HOST FRAMESYNC: No clients to send to - SKIPPING");
            return;
        }

        auto states = build_states_for_ids(targetIds);
        PacketStateHeader header;
        header.type = PKT_STATE;
        header.seq = m_seq++;
        header.objectCount = (uint32_t)states.size();

        std::vector<char> buf(sizeof(header) + states.size() * sizeof(ObjectState));
        memcpy(buf.data(), &header, sizeof(header));
        memcpy(buf.data() + sizeof(header), states.data(), states.size() * sizeof(ObjectState));

        netlog_printf("HOST FRAMESYNC: Broadcasting %zu states to %zu clients", states.size(), m_clients.size());
        for (const auto& c : m_clients) {
            bool sendSuccess = m_net.send_to(c.ip, c.port, buf.data(), (int)buf.size());
            netlog_printf("HOST FRAMESYNC: Sent to client %s:%d (id=%u) success=%d",
                c.ip.c_str(), c.port, c.playerId, sendSuccess ? 1 : 0);
        }

    } else {
        // Client: send its local players (ids 1 and 2) to the host
        if (m_hostIp.empty()) {
            netlog_printf("CLIENT FRAMESYNC: hostIP is empty - SKIPPING");
            return;
        }
        //�d�v: JOIN/ACK ���󂯂ăv���C���[ID�����蓖�Ă���܂�STATE���M���s��Ȃ�
        if (m_myPlayerId ==0) {
            netlog_printf("CLIENT FRAMESYNC: myPlayerId is0 (not joined) - SKIPPING");
            return;
        }
        auto states = build_states_for_ids(targetIds);
        PacketStateHeader header;
        header.type = PKT_STATE;
        header.seq = m_seq++;
        header.objectCount = (uint32_t)states.size();

        std::vector<char> buf(sizeof(header) + states.size() * sizeof(ObjectState));
        memcpy(buf.data(), &header, sizeof(header));
        memcpy(buf.data() + sizeof(header), states.data(), states.size() * sizeof(ObjectState));

        bool sendSuccess = m_net.send_to(m_hostIp, m_hostPort, buf.data(), (int)buf.size());
        netlog_printf("CLIENT FRAMESYNC SEND: Sent %zu states to host %s:%d success=%d",
            states.size(), m_hostIp.c_str(), m_hostPort, sendSuccess ? 1 : 0);
    }
}

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

bool NetworkManager::try_dynamic_ports() {
    return m_net.initialize_dynamic_port() && m_discovery.initialize_dynamic_port();
}

// check_existing_firewall_rule: �ŏ����ɂ��Ă���
bool NetworkManager::check_existing_firewall_rule() {
    // �R�X�g�̍���COM�񋓂͔����� false ��Ԃ��Ă����iadd_firewall_exception �ł͊Ǘ��Ҕ��肾���ŏI���j
    return false;
}

// add_firewall_exception: �Ǘ��җL�������`�F�b�N���ďd������͂��Ȃ�
bool NetworkManager::add_firewall_exception() {
    // �Ǘ��҃`�F�b�N���s�����A���ۂ�COM�Œǉ����鏈���͍s��Ȃ��i�w�Z���ł͑����̏ꍇ�s�j
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

// handle_channel_scan / handle_channel_info: �y�ʂɕێ�
void NetworkManager::handle_channel_scan(const std::string& from_ip, int from_port) {
    ChannelInfo info;
    info.type = PKT_CHANNEL_INFO;
    info.channelId = static_cast<uint32_t>(m_currentChannel);
    info.userCount = static_cast<uint32_t>(m_clients.size());
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

// Worker: polls sockets and pushes received packets to the queue.
// It also performs lightweight discovery replies immediately and does round-robin state sends.
void NetworkManager::start_worker() {
    if (m_workerRunning.exchange(true)) return; // already running
    m_worker = std::thread([this]() {
        std::vector<GameObject*> worldObjectsSnapshot; // worker needs a pointer to world objects when sending state
        auto lastStateSend = std::chrono::steady_clock::now();
        while (m_workerRunning.load()) {
            // Poll main net socket
            char buf[MAX_UDP_PACKET];
            std::string from_ip;
            int from_port = 0;

            // Poll frequently but non-blocking with short timeout
            int r = m_net.poll_recv(buf, sizeof(buf), from_ip, from_port, 50);
            if (r > 0) {
                // Push packet to queue for main thread to process (game logic)
                RecvPacket pkt;
                pkt.data.assign(buf, buf + r);
                pkt.len = r;
                pkt.from_ip = from_ip;
                pkt.from_port = from_port;
                pkt.isDiscovery = false;
                push_recv_packet(std::move(pkt));
            }

            // Poll discovery socket
            int dr = m_discovery.poll_recv(buf, sizeof(buf), from_ip, from_port, 10);
            if (dr > 0) {
                uint8_t dt = (uint8_t)buf[0];
                if (m_isHost && dt == PKT_DISCOVER) {
                    // reply immediately (very lightweight)
                    uint8_t reply = PKT_DISCOVER_REPLY;
                    m_discovery.send_to(from_ip, from_port, &reply, 1);
                } else {
                    // non-discovery packets (channel info etc.) push to queue
                    RecvPacket pkt;
                    pkt.data.assign(buf, buf + dr);
                    pkt.len = dr;
                    pkt.from_ip = from_ip;
                    pkt.from_port = from_port;
                    pkt.isDiscovery = true;
                    push_recv_packet(std::move(pkt));
                }
            }

            // Periodic state send (round-robin). Worker will attempt to send one client state every m_stateInterval.
            auto now = std::chrono::steady_clock::now();
            if (!m_isHost) {
                // nothing
            } else if (std::chrono::duration_cast<std::chrono::milliseconds>(now - lastStateSend) >= m_stateInterval) {
                // We need worldObjects reference to extract positions; to avoid data race, we keep a pointer to a small snapshot
                // Instead of reading worldObjects here (unsafe), we ask main thread to keep positions minimal; to keep this self-contained,
                // we will send empty/default state for the chosen client (keeps network alive). For correctness, main-thread send is better.
                // To keep light, send small ping-style state for one client.
                std::lock_guard<std::mutex> lk(m_mutex);
                if (!m_clients.empty()) {
                    size_t idx = m_stateSendIndex % m_clients.size();
                    ClientInfo& c = m_clients[idx];
                    // Build minimal state (only id + zeros)
                    PacketStateHeader header;
                    header.type = PKT_STATE;
                    header.seq = m_seq++;
                    header.objectCount = 1;
                    ObjectState os = {};
                    os.id = c.playerId;
                    os.posX = os.posY = os.posZ = 0.0f;
                    os.rotX = os.rotY = os.rotZ = 0.0f;

                    std::vector<char> sendbuf;
                    sendbuf.resize(sizeof(header) + sizeof(os));
                    memcpy(sendbuf.data(), &header, sizeof(header));
                    memcpy(sendbuf.data() + sizeof(header), &os, sizeof(os));
                    m_net.send_to(c.ip, c.port, sendbuf.data(), static_cast<int>(sendbuf.size()));
                    m_stateSendIndex = (m_stateSendIndex + 1) % (m_clients.empty() ? 1 : m_clients.size());
                }
                lastStateSend = now;
            }
            // Sleep briefly to avoid busy loop
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
        // worker exit
        });
}

void NetworkManager::stop_worker() {
    if (!m_workerRunning.exchange(false)) return;
    if (m_worker.joinable()) {
        m_worker.join();
    }
}

// push_recv_packet: push into deque with capacity limit to avoid memory blowup
void NetworkManager::push_recv_packet(RecvPacket&& pkt) {
    std::lock_guard<std::mutex> lk(m_recvMutex);
    const size_t MAX_QUEUE = 1024;
    if (m_recvQueue.size() >= MAX_QUEUE) {
        // drop oldest
        m_recvQueue.pop_front();
    }
    m_recvQueue.push_back(std::move(pkt));
}

// initialize_with_fallback unchanged except kept lightweight
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

void NetworkManager::send_state_to_all(std::vector<GameObject*>& worldObjects) {
    std::lock_guard<std::mutex> lk(m_mutex);
    if (m_clients.empty()) {
        netlog_printf("HOST SEND_STATE: No clients connected");
        return;
    }

    // Build state list: include host player (id==1) if present, plus all connected clients
    std::vector<ObjectState> states;

    // �z�X�g�v���C���[�iID=1�j��K�����[�J���v���C���[����擾
    GameObject* localPlayer = nullptr;
    localPlayer = GetLocalPlayerGameObject();

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

    // Add each connected client from worldObjects
    for (const auto& c : m_clients) {
        ObjectState os = {};
        os.id = c.playerId;
        bool found = false;
        for (auto* go : worldObjects) {
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

    PacketStateHeader header;
    header.type = PKT_STATE;
    header.seq = m_seq++;
    header.objectCount = (uint32_t)states.size();

    std::vector<char> buf(sizeof(header) + states.size() * sizeof(ObjectState));
    memcpy(buf.data(), &header, sizeof(header));
    memcpy(buf.data() + sizeof(header), states.data(), states.size() * sizeof(ObjectState));

    netlog_printf("HOST SEND_STATE: Broadcasting %zu states to %zu clients", states.size(), m_clients.size());
    for (const auto& c : m_clients) {
        bool sendSuccess = m_net.send_to(c.ip, c.port, buf.data(), (int)buf.size());
        netlog_printf("HOST SEND_STATE: Sent to client %s:%d (id=%u) success=%d",
            c.ip.c_str(), c.port, c.playerId, sendSuccess ? 1 : 0);
    }
}