#pragma once
// network_common.h
// 共通メッセージ定義など（パック指定）

#include <cstdint>

#pragma pack(push,1)
enum PacketType : uint8_t {
    PKT_DISCOVER = 1,       // ブロードキャストでホスト探索
    PKT_DISCOVER_REPLY = 2, // ホスト応答
    PKT_JOIN = 3,           // クライアント参加要求
    PKT_JOIN_ACK = 4,       // ホスト参加承認（playerId 返却）
    PKT_INPUT = 5,          // クライアント -> ホスト（入力）
    PKT_STATE = 6,          // ホスト -> クライアント（ゲーム状態）
    PKT_PING = 7,
    PKT_CHANNEL_SCAN = 8,   // チャンネル使用状況スキャン
    PKT_CHANNEL_INFO = 9    // チャンネル情報応答
};

// クライアント入力パケット（固定長）
struct PacketInput {
    uint8_t type;      // PKT_INPUT
    uint32_t seq;      // シーケンス番号
    uint32_t playerId;
    float moveX;
    float moveY;
    float moveZ;
    uint32_t buttons;  // ビットフラグ
};

// オブジェクトの状態（1エントリ分）
struct ObjectState {
    uint32_t id;
    float posX, posY, posZ;
    float rotX, rotY, rotZ;
};

// 状態パケットヘッダ（続いて ObjectState が objectCount 個続く）
struct PacketStateHeader {
    uint8_t type; // PKT_STATE
    uint32_t seq;
    uint32_t objectCount;
};

// チャンネル情報
struct ChannelInfo {
    uint8_t type; // PKT_CHANNEL_INFO
    uint32_t channelId;
    uint32_t userCount;
    uint32_t basePort;
    uint32_t discoveryPort;
};
#pragma pack(pop)

// 複数のポート範囲を定義（輻輳回避）
static const int PORT_RANGES[][3] = {
    {27777, 27778, 0},  // 現在のポート（チャンネル0）
    {28777, 28778, 1},  // 代替ポート1（チャンネル1）
    {29777, 29778, 2},  // 代替ポート2（チャンネル2）
    {30777, 30778, 3},  // 代替ポート3（チャンネル3）
    {31777, 31778, 4},  // 代替ポート4（チャンネル4）
    {32777, 32778, 5}   // 代替ポート5（チャンネル5）
};

static const int NUM_CHANNELS = sizeof(PORT_RANGES) / sizeof(PORT_RANGES[0]);
static const int NET_PORT = 27777;
static const int DISCOVERY_PORT = 27778;
static const int MAX_UDP_PACKET = 1400;

// 動的ポート範囲（ファイアウォール回避用）
static const int DYNAMIC_PORT_RANGES[][2] = {
    {49152, 65535},  // Windows動的ポート範囲
    {32768, 60999},  // Linux代替範囲
    {8000, 8999},    // HTTP代替範囲
    {9000, 9999}     // 一般的な代替範囲
};
static const int NUM_DYNAMIC_RANGES = sizeof(DYNAMIC_PORT_RANGES) / sizeof(DYNAMIC_PORT_RANGES[0]);