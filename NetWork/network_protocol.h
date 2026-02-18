#pragma once
// network_protocol.h - パケット処理・シリアライズユーティリティ

#include "network_common.h"
#include <vector>
#include <string>
#include <cstring>
#include <cstdint>

// スレッドセーフなネットワークログ出力（低頻度のみ使用）
void netlog_printf(const char* fmt, ...);

// パケット構築・解析ヘルパー
namespace NetworkProtocol {

    // STATEパケットを構築する
    std::vector<char> build_state_packet(uint32_t& seq, const std::vector<ObjectState>& states);

    // JOIN_ACKパケットを構築する
    void build_join_ack(uint8_t* out, uint32_t playerId);

    // STATEパケットを解析する（成功時true）
    bool parse_state_packet(const char* buf, int len, PacketStateHeader& outHdr, const ObjectState*& outEntries);

} // namespace NetworkProtocol
