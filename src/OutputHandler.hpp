#pragma once

#include <cstdint>

#include "vdunlib/net/IPv4Address.hpp"

#include "MaltBeaconHdr.hpp"
#include "PacketInfo.hpp"
#include "Config.hpp"
#include "RxStats.hpp"

namespace malt {

class OutputHandler {
public:
    explicit OutputHandler(Config const& cfg)
    : cfg_{cfg} {}

    void showTimeout(uint64_t);

    void showRcvdPacket(PacketInfo const&);

    void showSentPacket(MaltBeaconHdr const&);

    void showRxStats(RxStats const&);

    void showTxStats(uint64_t);
private:
    Config const& cfg_;
};

} // namespace malt