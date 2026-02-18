// network_protocol.cpp - パケット処理・シリアライズ実装
#include "pch.h"
#include "network_protocol.h"
#include <fstream>
#include <mutex>
#include <cstdarg>
#include <chrono>

// スレッドセーフなネットワークログ（低頻度のみログ出力）
static std::mutex g_netlog_mutex;
void netlog_printf(const char* fmt, ...) {
    std::lock_guard<std::mutex> lk(g_netlog_mutex);
    char buf[1024];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);

    // タイムスタンプ（短縮）
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

namespace NetworkProtocol {

    // STATEパケット構築
    std::vector<char> build_state_packet(uint32_t& seq, const std::vector<ObjectState>& states) {
        PacketStateHeader header;
        header.type = PKT_STATE;
        header.seq = seq++;
        header.objectCount = static_cast<uint32_t>(states.size());

        std::vector<char> buf(sizeof(header) + states.size() * sizeof(ObjectState));
        memcpy(buf.data(), &header, sizeof(header));
        if (!states.empty()) {
            memcpy(buf.data() + sizeof(header), states.data(), states.size() * sizeof(ObjectState));
        }
        return buf;
    }

    // JOIN_ACKパケット構築（out は最低5バイトのバッファ）
    void build_join_ack(uint8_t* out, uint32_t playerId) {
        out[0] = PKT_JOIN_ACK;
        uint32_t pid_net = htonl(playerId);
        memcpy(out + 1, &pid_net, 4);
    }

    // STATEパケット解析
    bool parse_state_packet(const char* buf, int len, PacketStateHeader& outHdr, const ObjectState*& outEntries) {
        if (len < static_cast<int>(sizeof(PacketStateHeader))) return false;
        memcpy(&outHdr, buf, sizeof(outHdr));
        size_t expected = sizeof(outHdr) + static_cast<size_t>(outHdr.objectCount) * sizeof(ObjectState);
        if (len < static_cast<int>(expected)) return false;
        outEntries = reinterpret_cast<const ObjectState*>(buf + sizeof(outHdr));
        return true;
    }

} // namespace NetworkProtocol
