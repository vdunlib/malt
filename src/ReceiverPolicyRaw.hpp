#pragma once

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <linux/ip.h>
#include <linux/udp.h>
#include <cstring>

#include "PacketInfo.hpp"
#include "AppUtils.hpp"
#include "Config.hpp"

namespace malt {

struct ReceiverPolicyRaw final {

    static int openSocket() {
        int s = socket(AF_INET, SOCK_RAW, IPPROTO_UDP);

        if (s == -1) {
            if (errno == EPERM)
                error("permission to receive multicast on "
                      "all UDP ports denied by host");
            else sysCallError("unable to create raw UDP socket");
        }

        return s;
    }

    static bool configureSocket(int) { return true; }

    static ReceivedPacket receivePacket(
            int s, PacketInfo& pinfo, Config const& cfg, uint64_t pktTs) {
        uint8_t buf[BufferSize];
        ssize_t rv = recv(s, buf, sizeof(buf), 0);

        if (rv == -1) {
            sysCallError("failed to read UDP packet");
            return ReceivedPacket::Failed;
        }

        if (rv == 0) {
            warningTs(pktTs, "no data received");
            return ReceivedPacket::Filtered;
        }

        auto rcvSize = static_cast<size_t>(rv);
        // Make sure the IP header fits into the received packet data,
        // but this should never fail
        if (rcvSize < sizeof(iphdr)) {
            warningTs(pktTs,
                    "received packet size ", rcvSize,
                    " is smaller than the minimal IP header size (",
                    sizeof(iphdr), ")");
            return ReceivedPacket::Filtered;
        }
        auto ipHdr = reinterpret_cast<iphdr*>(buf);

        // Make sure this packet is destined for the multicast group we're
        // interested in
        if (ipHdr->daddr != cfg.group().to_nl())
            return ReceivedPacket::Filtered;

        auto ipHdrLen = static_cast<uint16_t>(ipHdr->ihl) << 2u;
        auto udpPayloadOffset = ipHdrLen + sizeof(udphdr);
        // Make sure there is enough room for the UDP header and payload
        if (udpPayloadOffset > rcvSize) {
            warningTs(pktTs,
                    "UDP payload offset ", udpPayloadOffset,
                    " is outside of the received packet size ", rcvSize,
                    " (IP header len = ", ipHdrLen, ")");
            return ReceivedPacket::Filtered;
        }

        // The multicast group is prepopulated the by MaltReceiver
        // pinfo_.group = cfg_.group();
        pinfo.source = net::IPv4Address::from_nl(ipHdr->saddr);
        pinfo.ttl = static_cast<int16_t>(ipHdr->ttl);

        auto udpHdr = reinterpret_cast<udphdr*>(buf + ipHdrLen);
        pinfo.sport = ntohs(udpHdr->source);
        pinfo.dport = ntohs(udpHdr->dest);
        // This value maybe 0
        pinfo.payloadSize =
                static_cast<size_t>(ntohs(udpHdr->len)) - sizeof(udphdr);

        // This is a sanity check for the return value of recv()
        // vs the UDP payload size in the UDP header
        if (udpPayloadOffset + pinfo.payloadSize != rcvSize) {
            if (udpPayloadOffset + pinfo.payloadSize < rcvSize) {
                warningTs(pktTs,
                        "UDP datagram ", pinfo.source, ':',
                        pinfo.sport, " -> ",
                        pinfo.group, ':', pinfo.dport,
                        " has extraneous bytes (recv size = ",
                        rcvSize - udpPayloadOffset,
                        ", UDP size = ", pinfo.payloadSize, ')');
            } else {
                warningTs(pktTs,
                        "size of UDP datagram ", pinfo.source, ':',
                        pinfo.sport, " -> ",
                        pinfo.group, ':', pinfo.dport,
                        " is not suffient for UDP payload (recv size = ",
                        rcvSize - udpPayloadOffset,
                        ", UDP size = ", pinfo.payloadSize, ')');
                pinfo.payloadSize = rcvSize - udpPayloadOffset;
            }
        }

        memcpy(pinfo.payload, buf + udpPayloadOffset, pinfo.payloadSize);
        return ReceivedPacket::Accepted;
    }
};

} // namespace malt