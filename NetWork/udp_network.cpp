// udp_network.cpp
#include "udp_network.h"
#include <iostream>
#include <cstring>

std::random_device UdpNetwork::rd;
std::mt19937 UdpNetwork::gen(UdpNetwork::rd());

UdpNetwork::UdpNetwork() {}
UdpNetwork::~UdpNetwork() { close_socket(); }

bool UdpNetwork::is_valid() const { return sock != INVALID_SOCKET; }

bool UdpNetwork::initialize(int bind_port) {
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        std::cerr << "WSAStartup failed\n";
        return false;
    }

    sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock == INVALID_SOCKET) {
        std::cerr << "socket() failed\n";
        WSACleanup();
        return false;
    }

    sockaddr_in addr;
    ZeroMemory(&addr, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons((unsigned short)bind_port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(sock, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
        std::cerr << "bind() failed on port " << bind_port << "\n";
        closesocket(sock);
        sock = INVALID_SOCKET;
        WSACleanup();
        return false;
    }

    current_port = bind_port;

    // ブロッキングモードのまま select を使う（ioctlsocketで非ブロックにしない）
    u_long mode = 0;
    ioctlsocket(sock, FIONBIO, &mode);

    is_broadcast_socket = false;
    return true;
}

bool UdpNetwork::initialize_broadcast(int bind_port) {
    if (!initialize(bind_port)) return false;
    BOOL bOpt = TRUE;
    if (setsockopt(sock, SOL_SOCKET, SO_BROADCAST, (char*)&bOpt, sizeof(bOpt)) == SOCKET_ERROR) {
        std::cerr << "setsockopt SO_BROADCAST failed\n";
        return false;
    }
    is_broadcast_socket = true;
    return true;
}

bool UdpNetwork::initialize_dynamic_port() {
    int port = get_random_available_port();
    if (port < 0) {
        std::cerr << "No available dynamic port found\n";
        return false;
    }

    std::cout << "Using dynamic port: " << port << std::endl;
    return initialize(port);
}

bool UdpNetwork::send_to(const std::string& ip, int port, const void* data, int len) {
    if (sock == INVALID_SOCKET) return false;
    sockaddr_in dest;
    ZeroMemory(&dest, sizeof(dest));
    dest.sin_family = AF_INET;
    dest.sin_port = htons((unsigned short)port);
    inet_pton(AF_INET, ip.c_str(), &dest.sin_addr);

    int sent = sendto(sock, (const char*)data, len, 0, (sockaddr*)&dest, sizeof(dest));
    return sent == len;
}

bool UdpNetwork::send_broadcast(int port, const void* data, int len) {
    if (sock == INVALID_SOCKET) return false;
    return send_to("255.255.255.255", port, data, len);
}

int UdpNetwork::poll_recv(char* buffer, int bufferSize, std::string& from_ip, int& from_port, int timeout_ms) {
    if (sock == INVALID_SOCKET) return -1;

    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(sock, &readfds);

    timeval tv;
    tv.tv_sec = timeout_ms / 1000;
    tv.tv_usec = (timeout_ms % 1000) * 1000;

    int sel = select((int)sock + 1, &readfds, nullptr, nullptr, (timeout_ms >= 0) ? &tv : nullptr);
    if (sel <= 0) return 0;

    sockaddr_in from;
    int fromlen = sizeof(from);
    int ret = recvfrom(sock, buffer, bufferSize, 0, (sockaddr*)&from, &fromlen);
    if (ret <= 0) return 0;

    char ipbuf[INET_ADDRSTRLEN] = { 0 };
    inet_ntop(AF_INET, &from.sin_addr, ipbuf, sizeof(ipbuf));
    from_ip = ipbuf;
    from_port = ntohs(from.sin_port);

    return ret;
}

void UdpNetwork::close_socket() {
    if (sock != INVALID_SOCKET) {
        closesocket(sock);
        sock = INVALID_SOCKET;
    }
    WSACleanup();
}

// ユーティリティ関数の実装
bool UdpNetwork::is_port_available(int port) {
    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);

    SOCKET testSocket = socket(AF_INET, SOCK_DGRAM, 0);
    if (testSocket == INVALID_SOCKET) {
        WSACleanup();
        return false;
    }

    sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    bool available = (bind(testSocket, (sockaddr*)&addr, sizeof(addr)) == 0);
    closesocket(testSocket);
    WSACleanup();
    return available;
}

int UdpNetwork::get_random_available_port() {
    for (int rangeIdx = 0; rangeIdx < NUM_DYNAMIC_RANGES; rangeIdx++) {
        int min_port = DYNAMIC_PORT_RANGES[rangeIdx][0];
        int max_port = DYNAMIC_PORT_RANGES[rangeIdx][1];

        std::uniform_int_distribution<> dis(min_port, max_port);

        for (int attempts = 0; attempts < 100; attempts++) {
            int port = dis(gen);
            if (is_port_available(port)) {
                return port;
            }
        }
    }
    return -1; // 利用可能なポートが見つからない
}

std::vector<int> UdpNetwork::get_available_port_list(int count) {
    std::vector<int> ports;

    for (int i = 0; i < count; i++) {
        int port = get_random_available_port();
        if (port > 0) {
            ports.push_back(port);
        }
    }

    return ports;
}

std::string UdpNetwork::get_local_ip() {
    char hostname[256];
    if (gethostname(hostname, sizeof(hostname)) == SOCKET_ERROR) {
        return "127.0.0.1";
    }

    struct addrinfo hints = {}, * result;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;

    if (getaddrinfo(hostname, NULL, &hints, &result) != 0) {
        return "127.0.0.1";
    }

    char ip_str[INET_ADDRSTRLEN];
    struct sockaddr_in* addr_in = (struct sockaddr_in*)result->ai_addr;
    inet_ntop(AF_INET, &(addr_in->sin_addr), ip_str, INET_ADDRSTRLEN);

    freeaddrinfo(result);
    return std::string(ip_str);
}