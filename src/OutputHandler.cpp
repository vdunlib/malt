#include <vector>
#include <string>
#include <algorithm>
#include <cstring>

#include <fmt/format.h>

#include "vdunlib/net/IPv4Address.hpp"
#include "vdunlib/formatters/IPv4Formatters.hpp"
#include "vdunlib/formatters/NanosText.hpp"
#include "vdunlib/unix/terminal-colors.hpp"

#include "MaltBeaconHdr.hpp"
#include "AppUtils.hpp"
#include "OutputHandler.hpp"

namespace malt {

namespace {

std::string fmtTtl(int16_t ttl) {
    if (ttl != -1) return fmt::format("{}", ttl);
    return "?";
}

bool showMaltPacket(PacketInfo const& pinfo, bool colors) {
    if (pinfo.payloadSize <= sizeof(MaltBeaconHdr)) return false;

    auto hdr = reinterpret_cast<MaltBeaconHdr const*>(pinfo.payload);
    if (hdr->magic != MaltMagic) return false;

    if (pinfo.payloadSize != sizeof(MaltBeaconHdr) + hdr->dataLen)
        return false;

    char const* s = reinterpret_cast<char const*>(
            pinfo.payload + sizeof(MaltBeaconHdr));
    std::string sourceName{
        s, static_cast<std::string::size_type>(hdr->dataLen)};

    fmt::memory_buffer buf{};
    if (colors) fmt::format_to(buf, TERM_COLOR_RED_BRIGHT);
    fmt::format_to(buf,
            "{:<12} {}:{}->{}:{} TTL {}, malt pkt seq #{} | {} {}",
            strTs(pinfo.timestamp),
            pinfo.source, pinfo.sport, pinfo.group, pinfo.dport,
            fmtTtl(pinfo.ttl),
            hdr->seq, sourceName, strTs(hdr->timeNs));
    if (colors) fmt::format_to(buf, TERM_COLOR_RESET);
    fmt::print("{}\n", fmt::to_string(buf));
    return true;
}

void showPacketInfo(PacketInfo const& pinfo, bool colors) {
    fmt::memory_buffer buf{};
    if (colors) fmt::format_to(buf, TERM_COLOR_YELLOW_BRIGHT);

    fmt::format_to(buf,
            "{:<12} {}:{}->{}:{} TTL {}",
            strTs(pinfo.timestamp),
            pinfo.source, pinfo.sport, pinfo.group, pinfo.dport,
            fmtTtl(pinfo.ttl));

    if (colors) fmt::format_to(buf, TERM_COLOR_RESET);
    fmt::print("{}\n", fmt::to_string(buf));
}

void showPayload(PacketInfo const& pinfo, bool colors) {
    fmt::memory_buffer buf{};
    if (colors) fmt::format_to(buf, TERM_COLOR_YELLOW);

    for (unsigned start = 0; start < pinfo.payloadSize; start += 16) {
        unsigned end =
                pinfo.payloadSize - start < 16 ? pinfo.payloadSize : start + 16;

        fmt::format_to(buf, "  ");

        // hex view
        for (auto i = start; i < end; i++) {
            fmt::format_to(buf,
                           "{:02x} ", static_cast<unsigned>(pinfo.payload[i]));
            if (i - start == 8)
                fmt::format_to(buf, " ");
        }

        // If the last row is shorter than 16 characters fill in
        // the missing hex values with blanks. Nothing needs to
        // be done with the payload_size is multiple of 16
        if (end == pinfo.payloadSize && pinfo.payloadSize % 16 != 0) {
            for (auto i = pinfo.payloadSize % 16; i < 16; i++) {
                if (i == 8) fmt::format_to(buf, " ");
                fmt::format_to(buf, "   ");
            }
        }

        fmt::format_to(buf, " ");

        // char view
        for (auto i = start; i < end; i++) {
            if (std::isprint(pinfo.payload[i]))
                fmt::format_to(buf,
                               "{}", static_cast<char>(pinfo.payload[i]));
            else fmt::format_to(buf, ".");
        }
        if (end != pinfo.payloadSize)
            fmt::format_to(buf, "\n");
    }

    if (colors) fmt::format_to(buf, TERM_COLOR_RESET);
    fmt::print("{}\n", fmt::to_string(buf));
}

struct FlowStatsView {
    std::string source;
    std::string dport;
    std::string packets;
    std::string bytes;
    std::string aps;
    std::string rate;
};

char const* const CapSource{"Source"};
char const* const CapDPort{"DPort"};
char const* const CapPkts{"Pkts"};
char const* const CapBytes{"Bytes"};
char const* const CapAPS{"APS"};
char const* const CapRate{"Rate"};

FlowStatsView flowStatsView(
        net::IPv4Address source, uint16_t sport, uint16_t dport,
        FlowStats const& flowStats, uint64_t duration) {
    FlowStatsView fsv;
    fsv.source = fmt::format("{}:{}", source, sport);
    fsv.dport = fmt::format("{}", dport);
    fsv.packets = fmt::format("{}", flowStats.pkts());
    fsv.bytes = fmt::format("{}", flowStats.bytes());
    fsv.aps = fmt::format("{}", flowStats.avgPktSize());

    double rate =
            static_cast<double>(flowStats.bytes() << 3u)
            * 1'000'000'000 / duration;
    if (rate < 1000)
        fsv.rate = fmt::format("{:.2f}bps", rate);
    else if (rate < 1'000'000)
        fsv.rate = fmt::format("{:.2f}Kbps", rate/1'000);
    else if (rate < 1'000'000'000)
        fsv.rate = fmt::format("{:.2f}Mbps", rate/1'000'000);
    else fsv.rate = fmt::format("{:.2f}Gbps", rate/1'000'000'000);

    return fsv;
}

std::string sep(std::size_t len) {
    std::string s;
    s.reserve(len + 1);
    for (std::size_t i{0}; i < len; ++i)
        s.append("=");
    return s;
}

std::string fmtGrpDPort(
        net::IPv4Address group, uint dport, bool wildcard) {
    if (wildcard) return fmt::format("{}:*", group);
    return fmt::format("{}:{}", group, dport);
}

std::string rcvdDur(uint64_t duration) {
    NanosText nt;
    uint64_t nanos = duration % 1'000'000'000ul;
    uint64_t secs = duration / 1'000'000'000ul + nt.prc(nanos, 3);
    return fmt::format("{}.{}", secs, nt.buf);
}

void fmtRxStats(
        net::IPv4Address group, uint dport, bool wildcard,
        RxStats const& rxStats, fmt::memory_buffer& buf) {
    if (rxStats.size() == 0) {
        fmt::format_to(buf,
                "No traffic received for {} in {} sec",
                fmtGrpDPort(group, dport, wildcard),
                rcvdDur(rxStats.durationNanos()));
        return;
    }

    std::size_t sourceFldLen = strlen(CapSource);
    std::size_t dportFldLen = strlen(CapDPort);
    std::size_t pktsFldLen = strlen(CapPkts);
    std::size_t bytesFldLen = strlen(CapBytes);
    std::size_t apsFldLen = strlen(CapAPS);
    std::size_t rateFldLen = strlen(CapRate);

    std::vector<FlowStatsView> fsvs;
    fsvs.reserve(rxStats.size());
    rxStats.sortedForEach(
            [&fsvs, duration=rxStats.durationNanos(),
             &sourceFldLen, &dportFldLen, &pktsFldLen,
             &bytesFldLen, &apsFldLen, &rateFldLen]
            (auto source, auto sport, auto dport, auto const& fs){
            fsvs.emplace_back(flowStatsView(source, sport, dport, fs, duration));
            FlowStatsView const& fsv = fsvs.back();

            sourceFldLen = std::max(sourceFldLen, fsv.source.length());
            dportFldLen = std::max(dportFldLen, fsv.dport.length());
            pktsFldLen = std::max(pktsFldLen, fsv.packets.length());
            bytesFldLen = std::max(bytesFldLen, fsv.bytes.length());
            apsFldLen = std::max(apsFldLen, fsv.aps.length());
            rateFldLen = std::max(rateFldLen, fsv.rate.length());
    });

    char fmtStr[256];

    auto fmtStrEnd = fmt::format_to(fmtStr,
            "{{:<{}}} {{:<{}}} {{:>{}}} {{:>{}}} {{:>{}}} {{:>{}}}\n",
            sourceFldLen, dportFldLen, pktsFldLen, bytesFldLen,
            apsFldLen, rateFldLen);
    *fmtStrEnd = '\0';

    fmt::format_to(buf,
            "Traffic received for {} in {} sec\n",
            fmtGrpDPort(group, dport, wildcard),
            rcvdDur(rxStats.durationNanos()));

    fmt::format_to(buf, fmtStr, 
            CapSource, CapDPort, CapPkts, CapBytes, CapAPS, CapRate);

    fmt::format_to(buf, fmtStr,
            sep(sourceFldLen), sep(dportFldLen), sep(pktsFldLen),
            sep(bytesFldLen), sep(apsFldLen), sep(rateFldLen));

    for (auto const& fsv: fsvs)
        fmt::format_to(buf, fmtStr,
                fsv.source, fsv.dport, fsv.packets,
                fsv.bytes, fsv.aps, fsv.rate);
}

} // anon.namespace

void OutputHandler::showTimeout(uint64_t ts) {
    fmt::memory_buffer buf{};
    if (cfg_.colors()) fmt::format_to(buf, TERM_COLOR_WHITE_BRIGHT);
    fmt::format_to(buf, "{:<12} timeout", strTs(ts));
    if (cfg_.colors()) fmt::format_to(buf, TERM_COLOR_RESET);
    fmt::print("{}\n", fmt::to_string(buf));
}

void OutputHandler::showRcvdPacket(PacketInfo const& pinfo) {
    if (! showMaltPacket(pinfo, cfg_.colors()))
        showPacketInfo(pinfo, cfg_.colors());
    if (cfg_.showPayload()) showPayload(pinfo, cfg_.colors());
}

void OutputHandler::showSentPacket(MaltBeaconHdr const& hdr) {
    fmt::memory_buffer buf{};
    if (cfg_.colors()) fmt::format_to(buf, TERM_COLOR_RED_BRIGHT);
    fmt::format_to(buf,
            "{:<12} sent malt pkt seq #{}", strTs(hdr.timeNs), hdr.seq);
    if (cfg_.colors()) fmt::format_to(buf, TERM_COLOR_RESET);
    fmt::print("{}\n", fmt::to_string(buf));
}

void OutputHandler::showRxStats(RxStats const& rxStats) {
    fmt::memory_buffer buf{};
    if (cfg_.colors()) fmt::format_to(buf, TERM_COLOR_YELLOW_BRIGHT);
    fmtRxStats(cfg_.group(), cfg_.dport(), cfg_.wildcard(), rxStats, buf);
    if (cfg_.colors()) fmt::format_to(buf, TERM_COLOR_RESET);
    fmt::print("\n{}", fmt::to_string(buf));
}

void OutputHandler::showTxStats(uint64_t pktsSent) {
    fmt::memory_buffer buf{};
    if (cfg_.colors()) fmt::format_to(buf, TERM_COLOR_RED_BOLD);
    fmt::format_to(buf, "\nsent {} packets", pktsSent);
    if (cfg_.colors()) fmt::format_to(buf, TERM_COLOR_RESET);
    fmt::print("{}\n", fmt::to_string(buf));
}

} // namespace malt