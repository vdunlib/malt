#pragma once

#include <cstdint>
#include <string>

#include "vdunlib/net/IPv4Address.hpp"

using namespace vdunlib;

namespace malt {

class Config final {
public:
    static Config forArgs(int argc, char const* const* argv);

    Config(Config const&) = delete;
    Config(Config&&) noexcept = default;
    Config& operator= (Config const&) = delete;
    Config& operator= (Config&&) noexcept = default;

    net::IPv4Address group() const { return group_; }
    uint16_t dport() const { return dport_; }
    bool wildcard() const { return wildcard_; }
    std::string const& intf() const { return intf_; }
    net::IPv4Address intfAddr() const { return intfAddr_; }
    net::IPv4Address source() const { return source_; }
    unsigned timeoutSec() const { return timeoutSec_; }
    bool showPayload() const { return showPayload_; }
    bool sender() const { return sender_; }
    unsigned ttl() const { return ttl_; }
    uint64_t count() const { return count_; }
    bool colors() const { return colors_; }

    std::string str() const;
private:
    net::IPv4Address group_;
    uint16_t dport_;
    bool wildcard_;
    std::string intf_;
    net::IPv4Address intfAddr_;
    // If this is 0.0.0.0, we're subscribing to (*,G)
    // otherwise to (S,G) where S is the source_
    net::IPv4Address source_;
    unsigned timeoutSec_;
    bool sender_;
    unsigned ttl_;
    uint64_t count_;
    bool showPayload_;
    bool colors_;

    Config(net::IPv4Address group,
           uint16_t dport,
           bool wildcard,
           std::string intf,
           net::IPv4Address intfAddr,
           net::IPv4Address source,
           unsigned timeoutSec,
           bool sender,
           unsigned ttl,
           uint64_t count,
           bool showPayload,
           bool colors)
           : group_{group}
           , dport_{dport}
           , wildcard_{wildcard}
           , intf_{std::move(intf)}
           , intfAddr_{intfAddr}
           , source_{source}
           , timeoutSec_{timeoutSec}
           , sender_{sender}
           , ttl_{ttl}
           , count_{count}
           , showPayload_{showPayload}
           , colors_{colors} {}
};

} // namespace malt
