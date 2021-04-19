#pragma once

#include <cstdint>
#include "vdunlib/net/IPv4Address.hpp"

constexpr int BufferSize{67584};

namespace malt {

struct PacketInfo final {
    net::IPv4Address source;
    uint16_t sport;
    net::IPv4Address group;
    uint16_t dport;
    // If ttl field is -1, it means the receiver was unable
    // to get the TTL value
    int16_t ttl;
    // Unless packet display was requested or this packet was generated
    // by MaltSender this field may contain garbage
    uint8_t payload[BufferSize];
    unsigned payloadSize;
    uint64_t timestamp;
};

enum class ReceivedPacket {
    Accepted = 0,
    Filtered = 1,
    Failed = 2
};

} // namespace malt