#pragma once
// udp_network.h - シンプルなUDP送受信ラッパー（WinSock）

#include "network_common.h"
#include <winsock2.h>
#include <ws2tcpip.h>
#include <string>
#include <vector>
#include <random>

#pragma comment(lib, "Ws2_32.lib")

class UdpNetwork {
public:
    UdpNetwork();
    ~UdpNetwork();

    // 初期化 (bind 実行)
    bool initialize(int bind_port = NET_PORT);

    // ブロードキャスト用ソケット (探索に使用)
    bool initialize_broadcast(int bind_port = DISCOVERY_PORT);

    // 動的ポート初期化（ファイアウォール回避）
    bool initialize_dynamic_port();

    // 指定先へ送信
    bool send_to(const std::string& ip, int port, const void* data, int len);

    // ブロードキャスト送信（255.255.255.255）
    bool send_broadcast(int port, const void* data, int len);

    // 受信ポーリング: 戻り受信バイト数。timeout_ms=0 なら即座ブロッキング
    int poll_recv(char* buffer, int bufferSize, std::string& from_ip, int& from_port, int timeout_ms = 0);

    void close_socket();

    // 状態チェック
    bool is_valid() const;

    // ユーティリティ関数
    static bool is_port_available(int port);
    static int get_random_available_port();
    static std::vector<int> get_available_port_list(int count = 10);
    static std::string get_local_ip();

    // 現在のポート取得
    int get_current_port() const { return current_port; }

private:
    SOCKET sock = INVALID_SOCKET;
    bool is_broadcast_socket = false;
    int current_port = 0;

    static std::random_device rd;
    static std::mt19937 gen;
};