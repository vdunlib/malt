#pragma once

#include <cstdint>
#include <unordered_map>
#include <set>

#include "vdunlib/core/CompilerUtils.hpp"
#include "vdunlib/net/IPv4Address.hpp"
#include "vdunlib/time/Time.hpp"

namespace malt {

VDUNLIB_ALWAYS_INLINE
static constexpr uint64_t flowId(
        net::IPv4Address src, uint16_t sport, uint16_t dport) {
    return (static_cast<uint64_t>(dport) << 48u) +
           (static_cast<uint64_t>(src.value()) << 16u) +
           static_cast<uint64_t>(sport);
}

VDUNLIB_ALWAYS_INLINE
static constexpr net::IPv4Address flowSource(uint64_t flowId) {
    return net::IPv4Address{
        static_cast<uint32_t>((flowId >> 16u) & 0xFFFFFFFFul)};
}

VDUNLIB_ALWAYS_INLINE
static constexpr uint16_t flowSPort(uint64_t flowId) {
    return static_cast<uint16_t>(flowId & 0xFFFFu);
}

VDUNLIB_ALWAYS_INLINE
static constexpr uint16_t flowDPort(uint64_t flowId) {
    return static_cast<uint16_t>((flowId >> 48u) & 0xFFFFu);
}

class FlowStats final {
    constexpr static uint64_t withHeaders(uint64_t udpBytes) {
        // 12 bytes MAC header (we assume no VLAN)
        // 20 bytes IP header
        // 8 bytes UDP header
        // ... UDP payload size
        // 4 bytes FSC
        return 12u + 20u + 8u + udpBytes + 4u;
    }
public:
    constexpr explicit FlowStats(uint64_t udpBytes)
    : pkts_{1}, bytes_{withHeaders(udpBytes)} {}

    void add(uint64_t udpBytes) {
        ++pkts_;
        bytes_ += withHeaders(udpBytes);
    }

    uint64_t pkts() const { return pkts_; }
    uint64_t bytes() const { return bytes_; }
    unsigned avgPktSize() const { return bytes_ / pkts_; }

private:
    uint64_t pkts_;
    uint64_t bytes_;
};

class RxStats final {
public:
    class Timer final {
    public:
        explicit Timer(RxStats& rxStats)
        : startNanos_{TimeUtils::gethostnanos()}
        , durationNanos_{rxStats.durationNanos_} {}

        Timer(Timer const&) = delete;
        Timer(Timer&&) = delete;
        Timer& operator= (Timer const&) = delete;
        Timer& operator= (Timer&&) = delete;

        ~Timer() {
            durationNanos_ = TimeUtils::gethostnanos() - startNanos_;
        }
    private:
        uint64_t startNanos_;
        uint64_t& durationNanos_;
    };

    friend class RxStats::Timer;

    void update(net::IPv4Address source,
            uint16_t sport, uint16_t dport, uint64_t udpBytes) {
        auto fid = flowId(source, sport, dport);

        auto fme = fsMap_.emplace(fid, udpBytes);
        if (! fme.second)
            fme.first->second.add(udpBytes);
        else fids_.emplace(fid);
    }

    template <typename Consumer>
    void sortedForEach(Consumer&& consume) const {
        for (const auto& fid: fids_) {
            consume(flowSource(fid),
                    flowSPort(fid), flowDPort(fid),
                    fsMap_.find(fid)->second);
        }
    }

    uint64_t durationNanos() const { return durationNanos_; }

    std::size_t size() const { return fsMap_.size(); }

private:
    std::unordered_map<uint64_t, FlowStats> fsMap_;
    std::set<uint64_t> fids_;
    uint64_t durationNanos_;
};

}