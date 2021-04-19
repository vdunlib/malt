#pragma once

#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <cstring>
#include <thread>
#include <chrono>

#include "vdunlib/time/Time.hpp"
#include "vdunlib/formatters/IPv4Formatters.hpp"

#include "MaltBeaconHdr.hpp"
#include "AppUtils.hpp"
#include "Config.hpp"
#include "OutputHandler.hpp"
#include "Malt.hpp"
#include "MaltBase.hpp"

using namespace std::chrono_literals;

namespace malt {

class MaltSender final: public IMaltRunner, protected MaltBase {
    struct MaltBeaconPacket {
        MaltBeaconHdr hdr;
        char hostname[64];
    } __attribute__((__aligned__(1), __packed__));

public:
    MaltSender(
            Config const& cfg, OutputHandler& oh, bool& stopped)
    : MaltBase{cfg, oh, stopped} {
        pkt_.hdr.magic = MaltMagic;
        pkt_.hdr.seq = 0;
    }

    bool init() {
        if (gethostname(pkt_.hostname, sizeof(pkt_.hostname)) == -1)
            return sysCallError("unable to get host name");

        pkt_.hostname[sizeof(pkt_.hostname) - 1] = '\0';
        pkt_.hdr.dataLen = static_cast<uint8_t>(strlen(pkt_.hostname));
        pktSize_ = sizeof(MaltBeaconHdr) + pkt_.hdr.dataLen;

        s_ = socket(AF_INET, SOCK_DGRAM, 0);

        if (s_ == -1)
            sysCallError("unable to create socket");

        auto ttl = static_cast<u_char>(cfg_.ttl());
        if (setsockopt(s_, IPPROTO_IP,
                       IP_MULTICAST_TTL, &ttl, sizeof(ttl)) == -1)
            return error("unable to set TTL ", ttl, ": ", sysError(errno));

        // this is required to allow this host to receive its own packets
        u_char loopback{1};
        if (setsockopt(s_, IPPROTO_IP,
                       IP_MULTICAST_LOOP, &loopback, sizeof(loopback)) == -1)
            return error("unable to set loopback mode on socket");

        in_addr intfAddr { .s_addr = cfg_.intfAddr().to_nl() };
        if (setsockopt(s_, IPPROTO_IP,
            IP_MULTICAST_IF, &intfAddr, sizeof(intfAddr)) == -1)
            return error("unable to make ", cfg_.intf(),
                         " (addr ", cfg_.intfAddr(),
                         ") output multicast interface",
                         sysError(errno));

        return true;
    }

    bool run() final {
        if (! init())
            return false;
        
        bool r = tryRun();

        oh_.showTxStats(pkt_.hdr.seq);
        return r;
    }

private:
    MaltBeaconPacket pkt_;
    unsigned pktSize_;

    bool tryRun() {
        sockaddr_in dst{};
        dst.sin_family = AF_INET;
        dst.sin_port = htons(cfg_.dport());
        dst.sin_addr.s_addr = cfg_.group().to_nl();

        while (! stopped_) {
            pkt_.hdr.timeNs = TimeUtils::gethostnanos();
            if (sendto(s_, &pkt_, pktSize_, 0,
                    reinterpret_cast<sockaddr*>(&dst), sizeof(dst)) == -1) {
                return error(
                        "failed to send packet to ",
                        cfg_.group(), ':', cfg_.dport(), ": ",
                        sysError(errno));
            }
            
            oh_.showSentPacket(pkt_.hdr);
            ++pkt_.hdr.seq;

            if (cfg_.count() != 0 && pkt_.hdr.seq >= cfg_.count())
                return true;

            std::this_thread::sleep_for(1s);
        }

        // we're stopped
        return true;
    }
};

} // namespace malt