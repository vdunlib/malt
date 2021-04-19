#pragma once

#include <cstdint>

namespace malt {

constexpr uint64_t MaltMagic{5505949305068913751};

struct MaltBeaconHdr final {
    uint64_t magic;
    uint64_t seq;
    uint64_t timeNs;
    uint8_t dataLen;
} __attribute__((__aligned__(1), __packed__));

} // namespace malt