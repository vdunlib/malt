#pragma once

#include <sys/types.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "PacketInfo.hpp"
#include "AppUtils.hpp"
#include "Config.hpp"

namespace malt {

struct ReceiverPolicyReg final {

    static int openSocket() {
        int s = socket(AF_INET, SOCK_DGRAM, 0);

        if (s == -1)
            sysCallError("unable to create socket");

        return s;
    }

    static bool configureSocket(int s) {
        int ttl = 1;
        if (setsockopt(s, IPPROTO_IP, IP_RECVTTL, &ttl, sizeof(ttl)) == -1)
            return sysCallError("cannot enable receiving TTL");
        return true;
    }

    static ReceivedPacket receivePacket(
            int s, PacketInfo& pinfo, Config const& cfg, uint64_t) {
        iovec iov;
        iov.iov_base = pinfo.payload;
        iov.iov_len = sizeof(pinfo.payload);
        size_t cmsgSize = sizeof(cmsghdr) + sizeof(int16_t);
        uint8_t cmsgBuf[CMSG_SPACE(cmsgSize)];
        sockaddr_in sender;
        memset(&sender, 0, sizeof(sender));
        msghdr msg;
        memset(&msg, 0, sizeof(msg));
        msg.msg_name = &sender;
        msg.msg_namelen = sizeof(sender);
        msg.msg_iov = &iov;
        msg.msg_iovlen = 1;
        msg.msg_control = cmsgBuf;
        msg.msg_controllen = sizeof(cmsgBuf);

        ssize_t recvMsgSize = recvmsg(s, &msg, 0);

        if (recvMsgSize == -1) {
            sysCallError("unable to receive packet");
            return ReceivedPacket::Failed;
        }

        pinfo.payloadSize = static_cast<unsigned>(recvMsgSize);

        pinfo.ttl = -1;
        for (auto cmsg_ptr = CMSG_FIRSTHDR(&msg);
             cmsg_ptr != nullptr;
             cmsg_ptr = CMSG_NXTHDR(&msg, cmsg_ptr)) {
            if (cmsg_ptr->cmsg_level == IPPROTO_IP
                && cmsg_ptr->cmsg_type == IP_TTL
                && cmsg_ptr->cmsg_len > 0) {
                auto p = static_cast<void *>(CMSG_DATA(cmsg_ptr));
                pinfo.ttl = *static_cast<int16_t *>(p);
            }
        }

        pinfo.dport = cfg.dport();
        pinfo.source = net::IPv4Address::from_nl(sender.sin_addr.s_addr);
        pinfo.sport = ntohs(sender.sin_port);
        return ReceivedPacket::Accepted;
    }
};

} // namespace malt
