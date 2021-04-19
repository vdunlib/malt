#pragma once

#include "vdunlib/time/Time.hpp"

#include "Config.hpp"

namespace malt {

/**
 * This class facilitates timeout reporting. If we were only to
 * open a regular socket then we could use the timeout parameter
 * of the epoll_wait call to report timeouts. This unfortunately
 * won't work in case we open a raw UDP socket because then we
 * receive all UDP traffic destined for the host, thus we need to
 * accept only the multicast packets we're interested in.
 *
 * This class records the host's clock in nanoseconds every time
 * its reset() function is called and it reports if we exceeded
 * the timeout every time its bool cast operator is called. This
 * way when we manually reset the timeout only when a packet of
 * interest is received, we can accurately report timeouts on the
 * raw socket as well.
 */
class TimeoutCounter {
public:
    explicit TimeoutCounter(Config const& cfg)
    : startNs_{TimeUtils::gethostnanos()}
    , timestampNs_{startNs_}
    , timeoutNs_{static_cast<uint64_t>(cfg.timeoutSec()) * 1'000'000'000} {}

    /**
     * This function should be called right after epoll_wait to save
     * the host time at that moment
     */
    void timestamp() { timestampNs_ = TimeUtils::gethostnanos(); }

    void reset() { startNs_ =  timestampNs_; }

    operator bool() const {
        return timestampNs_ - startNs_ >= timeoutNs_;
    }

    uint64_t getTimestamp() const { return timestampNs_; }

private:
    uint64_t startNs_;
    uint64_t timestampNs_;
    uint64_t const timeoutNs_;
};

}