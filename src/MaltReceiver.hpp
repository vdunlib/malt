#pragma once

#include <fcntl.h>
#include <sys/epoll.h>

#include "vdunlib/time/Time.hpp"
#include "vdunlib/unix/SysError.hpp"

#include "PacketInfo.hpp"
#include "AppUtils.hpp"
#include "Config.hpp"
#include "OutputHandler.hpp"
#include "Malt.hpp"
#include "MaltBase.hpp"
#include "RxStats.hpp"
#include "TimeoutCounter.hpp"

namespace malt {

template <typename ReceiverPolicy>
class MaltReceiver final: public IMaltRunner, protected MaltBase {
public:

    MaltReceiver(
            Config const& cfg, OutputHandler& oh, bool& stopped)
    : MaltBase{cfg, oh, stopped}, epfd_{-1} {
        pinfo_.group = cfg_.group();
    }

    bool init() {
        s_ = ReceiverPolicy::openSocket();
        if (s_ == -1)
            return false;

        if (! configureSocket())
            return false;

        return activatePoller();
    }

    bool run() final {
        if (! init())
            return false;
        
        bool r = tryRun();

        oh_.showRxStats(rxStats);
        return r;
    }

private:
    int epfd_;
    PacketInfo pinfo_;
    RxStats rxStats;

    bool configureSocket() {
        // Make socket non-blocking
        int flags = fcntl(s_, F_GETFL);
        if (flags == -1)
            return sysCallError("fcntl() failed to get socket flags");

        flags |= O_NONBLOCK;
        int rv = fcntl(s_, F_SETFL, flags);
        if (rv == -1)
            return sysCallError("fcntl() failed to make socket non-blocking");

        // allow multiple sockets use the same UDP ports
        uint allowReuse = 1;
        if (setsockopt(s_, SOL_SOCKET, SO_REUSEADDR,
                       &allowReuse, sizeof(allowReuse)) == -1)
            return sysCallError("cannot enable UDP port reuse");

        int bufSize{BufferSize};
        if (setsockopt(s_, SOL_SOCKET,
                SO_RCVBUF, &bufSize, sizeof(bufSize)) == -1) {
            warning("failed to set receive buffer size to ",
                    bufSize, " bytes: ", sysError(errno));
        }

        if (! ReceiverPolicy::configureSocket(s_))
            return false;

        // Bind the socket to any interface. The join will be sent
        // from the interface specified on the command line
        sockaddr_in src{};
        memset(&src, 0, sizeof(src));
        src.sin_family = AF_INET;
        src.sin_port = htons(cfg_.dport());
        src.sin_addr.s_addr = INADDR_ANY;

        if (bind(s_, reinterpret_cast<sockaddr*>(&src), sizeof(src)) == -1)
            return error("cannot bind to UDP port ",
                         cfg_.dport(), ": ", sysError(errno));

        return true;
    }

    bool activatePoller() {
        epfd_ = epoll_create1(0);
        if (epfd_ == -1)
            return sysCallError("unable to create epoll instance");

        epoll_event ev{};
        ev.data.fd = s_;
        ev.events = EPOLLIN | EPOLLPRI;

        if (epoll_ctl(epfd_, EPOLL_CTL_ADD, s_, &ev) == -1)
            return sysCallError("unable to add socket to epoll instance");

        return true;
    }

    bool join() {
        if (cfg_.source() != net::IPv4Address{}) {
            ip_mreq_source mreq_source{};
            mreq_source.imr_interface.s_addr = cfg_.intfAddr().to_nl();
            mreq_source.imr_multiaddr.s_addr = cfg_.group().to_nl();
            mreq_source.imr_sourceaddr.s_addr = cfg_.source().to_nl();

            if (setsockopt(s_, IPPROTO_IP, IP_ADD_SOURCE_MEMBERSHIP,
                           &mreq_source, sizeof(mreq_source)) == -1)
                return error("failed to join (",
                             cfg_.source(),',', cfg_.group(), ") on ",
                             cfg_.intf(), ": ", sysError(errno));
        } else {
            ip_mreq mreq{};
            mreq.imr_interface.s_addr = cfg_.intfAddr().to_nl();
            mreq.imr_multiaddr.s_addr = cfg_.group().to_nl();

            if (setsockopt(s_, IPPROTO_IP,
                           IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) == -1)
                return error("failed to join (*,", cfg_.group(), ") on ",
                             cfg_.intf(), ": ", sysError(errno));
        }

        return true;
    }

    bool tryRun() {
        if (! join())
            return false;

        RxStats::Timer rxStatsTimer{rxStats};
        epoll_event rcvEv{};
        uint64_t count{0};
        TimeoutCounter timeout{cfg_};

        while (! stopped_) {
            int rc = epoll_wait(epfd_, &rcvEv, 1, 100);
            timeout.timestamp();

            if (rc == -1) {
                if (errno == EINTR)
                    continue;

                return sysCallError(
                        "failure while waiting on epoll instance");
            }

            if (rc == 0) {
                if (timeout) {
                    oh_.showTimeout(timeout.getTimestamp());
                    timeout.reset();
                }

                continue;
            }

            if (rcvEv.events & EPOLLHUP)
                return error("received epoll hangup");

            if (rcvEv.events & EPOLLERR) {
                int soError{0};
                socklen_t soErrorLen{sizeof(soError)};

                rc = getsockopt(s_,
                        SOL_SOCKET, SO_ERROR, &soError, &soErrorLen);

                if (rc == -1)
                    return sysCallError(
                            "epoll_wait indicated socket error, "
                            "but getsockopt() unabled to get the error code");

                return error("socket error: ", sysError(soError));
            }

            if ((rcvEv.events & EPOLLIN) || (rcvEv.events & EPOLLPRI)) {
                switch (ReceiverPolicy::receivePacket(
                        s_, pinfo_, cfg_, timeout.getTimestamp())) {
                case ReceivedPacket::Accepted:
                    pinfo_.timestamp = timeout.getTimestamp();
                    timeout.reset();
                    oh_.showRcvdPacket(pinfo_);
                    rxStats.update(
                            pinfo_.source, pinfo_.sport,
                            pinfo_.dport, pinfo_.payloadSize);
                    if (cfg_.count() > 0 && ++count > cfg_.count())
                        return true;
                    break;

                case ReceivedPacket::Filtered:
                    continue;

                case ReceivedPacket::Failed:
                    return false;
                }
            }
        }

        // we were stopped
        return true;
    }
};

} // namespace malt