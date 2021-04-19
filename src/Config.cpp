#include <cstdint>
#include <tuple>
#include <string>

#include <fmt/format.h>
#include <fmt/ostream.h>

#include <boost/program_options.hpp>

#include "vdunlib/parsers/NumberParsers.hpp"
#include "vdunlib/net/IPv4Address.hpp"
#include "vdunlib/parsers/IPv4Parsers.hpp"
#include "vdunlib/formatters/IPv4Formatters.hpp"
#include "vdunlib/utils/config/ParamDescrList.hpp"

#include "Config.hpp"
#include "AppUtils.hpp"
#include "IPv4IntfList.hpp"
// Generated file
#include "Version.hpp"

namespace po = boost::program_options;

namespace malt {

namespace {
struct GroupPort final {
    net::IPv4Address group;
    uint16_t dport;
    bool wildcard;
};

net::IPv4Address getGroup(std::string const& groupTxt) {
    net::IPv4Address group = parse<net::IPv4Address>(
            groupTxt,
            [&groupTxt] {
                appAbort("invalid multicast group '", groupTxt, "'");
            });

    if (! group.isMcast())
        appAbort("address ", group, " is not multicast");

    return group;
}

uint16_t getDPort(std::string const& portTxt) {
    uint64_t port = parseUInt64(portTxt,
            [&portTxt] {
                appAbort("invalid UDP port '", portTxt, "'");
            },
            [&portTxt] {
                appAbort("invalid UDP port ", portTxt);
            });
    if (port == 0 || port > 65535)
        appAbort("invalid UDP port ", port);

    return static_cast<uint16_t>(port);
}

GroupPort groupPort(
        bool targetSpecified,
        std::string const& groupPortTxt,
        bool portSpecified, std::string& portTxt) {
    if (! targetSpecified)
        appAbort("no multicast target specified");

    net::IPv4Address group{};
    uint16_t dport{0};
    bool wildcard{true};

    bool colonFound{false};
    unsigned colonPos{0};
    for (auto const& c: groupPortTxt) {
        if (c == ':') {
            colonFound = true;
            break;
        }
        ++colonPos;
    }

    if (colonFound) {
        group = getGroup(groupPortTxt.substr(0, colonPos));
        auto trgPortTxt = groupPortTxt.substr(colonPos + 1);
        if (trgPortTxt != "*") {
            dport = getDPort(trgPortTxt);
            wildcard = false;
        }
    } else {
        group = getGroup(groupPortTxt);
    }

    if (portSpecified) {
        if (colonFound)
            appAbort("option -p|--port may not be used if UDP port "
                     "is specified in the target");

        dport = getDPort(portTxt);
        wildcard = false;
    }

    return GroupPort{
        .group = group,
        .dport = dport,
        .wildcard = wildcard
    };
}

net::IPv4Address checkMCastIntf(
        bool intfSpecified, std::string const& intfTxt) {
    if (! intfSpecified)
        appAbort("the option '--intf' is required");

    for (const auto& intft: getIPv4IntfList()) {
        if (intfTxt == std::get<std::string>(intft))
            return std::get<net::IPv4Address>(intft);
    }

    appAbort("invalid IPv4 multicast interface '", intfTxt, "'");
    return net::IPv4Address{};
}

net::IPv4Address getSource(bool ssJoin, std::string const& sourceTxt) {
    if (! ssJoin) return net::IPv4Address{};

    auto source = parse<net::IPv4Address>(
            sourceTxt,
            [&sourceTxt] {
                appAbort("invalid source IP address '", sourceTxt, "'");
            });

    if (source.isMcast() ||
        source.isDefault() ||
        source.oct1() == 0 ||
        source.isLocalBroadcast())
        appAbort("invalid source IP address ", source);

    return source;
}

unsigned getTimeout(
        bool timeoutSpecified, std::string const& timeoutTxt) {
    if (! timeoutSpecified) return 5;

    auto timeout = parseUInt64(timeoutTxt,
            [&timeoutTxt] {
                appAbort("invalid timeout '", timeoutTxt, "'");
            },
            [& timeoutTxt] {
                appAbort("invalid timeout ", timeoutTxt);
            });
    if (timeout > 60)
        appAbort("invalid timeout ", timeout);

    return static_cast<unsigned>(timeout);
}

std::tuple<bool, unsigned> getSenderParams(
        bool senderSpecified, bool ttlSpecified, std::string const& ttlTxt) {
    if (! senderSpecified) {
        if (ttlSpecified)
            appAbort("--ttl may only be used with --sender");

        return std::make_tuple(false, 0u);
    }

    if (! ttlSpecified)
        return std::make_tuple(true, 255u);

    auto ttl = parseUInt64(ttlTxt,
            [&ttlTxt] {
                appAbort("invalid TTL '", ttlTxt, "'");
            },
            [&ttlTxt] {
                appAbort("invalid TTL ", ttlTxt);
            });
    if (ttl > 255)
        appAbort("invalid TTL ", ttl);

    return std::make_tuple(true, static_cast<unsigned>(ttl));
}

uint64_t getCount(bool countSpecified, std::string const& countTxt) {
    if (! countSpecified)
        return 0;

    return parseUInt64(countTxt,
            [&countTxt] {
                appAbort("invalid count '", countTxt, "'");
            },
            [&countTxt] {
                appAbort("invalid count ", countTxt);
            });
}

} // anon.namespace

Config Config::forArgs(int argc, char const* const* argv) {
    std::string groupPortTxt;
    po::options_description groupPortOpts{"Group and optional port"};
    groupPortOpts.add_options()
            ("group",
             po::value(&groupPortTxt)->value_name("<group[:port]>"));

    std::string udpPortTxt;
    std::string intfTxt;
    std::string sourceTxt;
    std::string timeoutSecTxt;
    std::string ttlTxt;
    std::string countTxt;
    po::options_description generalOpts{"Options"};
    generalOpts.add_options()
            ("help,h", "Print usage and exit")
            ("port,p", po::value(&udpPortTxt)->value_name("<UDP port>"),
             "Specify the UDP port to which the subscription is made. "
             "Alternatively, the UDP port can be specified as part of "
             "the positional parameter in the form G:P, where G is the "
             "multicast group and P is the port, e.g. 239.1.2.3:23456. "
             "If the port is not specified malt will attempt to receive "
             "traffic destined for the group and all UDP ports. This "
             "operation requires the CAP_NET_RAW capability for the malt "
             "process.")
            ("intf,i", po::value(&intfTxt)->value_name("<Interface>"),
             "Specify the multicast interface. This parameter is required")
            ("source,s", po::value(&sourceTxt)->value_name("<Source-IP>"),
             "Specify the multicast source IP address. If this option is "
             "present, malt will perform IGMPv3 source specific (S,G) join "
             "where S is the source and G is the group. The ability to "
             "perform IGMPv3 joins depends on the host configuration, if "
             "IGMPv3 is disabled, the host will issue an IGMPv2 join for "
             "(*,G) and it will filter the other sources before letting "
             "malt see the traffic.")
            ("timeout,t", po::value(&timeoutSecTxt)->value_name("<Timeout>"),
             "Specify a timeout in seconds for the received packets. "
             "If malt doesn't receive a packet in the specified number of "
             "seconds, it will log a message. The valid values are in range "
             "0-60, where 0 indicates no timeout reporting. Defaults to 5 "
             "sec.")
            ("sender",
             "Instead of subscribing to multicast, send multicast to the "
             "specified group and port. This option requires both a group "
             "and a UDP port to be specified. Malt will send one packet "
             "per second.")
            ("ttl", po::value(&ttlTxt)->value_name("<TTL>"),
             "If malt is sending, specify the TTL in the transmitted multicast "
             "packets. This option is available only if --sender is specified. "
             "Defaults to 255.")
            ("data,d",
             "Show UDP payload data in hexadecimal and printable ASCII")
            ("count,c", po::value(&countTxt)->value_name("<Count>"),
             "Limit the number of received packets. Once the specified number "
             "of packets is received, malt terminates and prints the stats.")
            ("nocolors",
             "Suppress colors in the output")
            ("version", "Print malt version and exit")
            ("show-config", "Show malt config");

    po::options_description allOpts{"All"};
    allOpts.add(groupPortOpts).add(generalOpts);
    po::positional_options_description posParams;
    posParams.add("group", 1);

    po::command_line_parser parser{argc, argv};
    parser.options(allOpts).positional(posParams);
    po::variables_map vm;
    try {
        po::parsed_options parsedOptions = parser.run();
        po::store(parsedOptions, vm);
        po::notify(vm);
    } catch (po::error const& err) {
        appAbort(err.what());
    }

    if (vm.count("help") > 0) {
        fmt::print(
                "Usage: malt -i <intf> <group>[:<UDP port>]\n"
                "            [-p|--port <UDP port>]\n"
                "            [-s|--source <Source-IP>]\n"
                "            [-t|--timeout <Timeout>]\n"
                "            [--sender]\n"
                "            [--ttl <TTL>]\n"
                "            [-d|--data]\n"
                "            [-c|--cout <Count>]\n"
                "            [--nocolors]\n"
                "            [--version]\n"
                "            [--show-config]\n\n{}\n", generalOpts);
        exit(0);
    }

    if (vm.count("version") > 0) {
        fmt::print(VERSION);
        exit(0);
    }

    auto gp = groupPort(
            vm.count("group") > 0, groupPortTxt,
            vm.count("port") > 0, udpPortTxt);
    auto intfAddr = checkMCastIntf(vm.count("intf") > 0, intfTxt);
    auto sourceAddr = getSource(vm.count("source") > 0, sourceTxt);
    auto timeoutSec = getTimeout(vm.count("timeout") > 0, timeoutSecTxt);
    bool showPayload = vm.count("data") > 0;
    auto count = getCount(vm.count("count") > 0, countTxt);
    bool nocolors = vm.count("nocolors") > 0;
    bool sender;
    unsigned ttl;
    std::tie(sender, ttl) = getSenderParams(
            vm.count("sender") > 0, vm.count("ttl") > 0, ttlTxt);
    if (sender) {
        if (gp.wildcard)
            appAbort("the UDP port is required in the sender mode");
        if (sourceAddr != net::IPv4Address{})
            appAbort("the source IP address may not be specified "
                     "in the sender mode");

        if (showPayload)
            appAbort("option -d|--data is not available in the sender mode");
    }

    Config cfg{
        gp.group,
        gp.dport,
        gp.wildcard,
        std::move(intfTxt),
        intfAddr,
        sourceAddr,
        timeoutSec,
        sender,
        ttl,
        count,
        showPayload,
        ! nocolors
    };

    if (vm.count("show-config") > 0)
        fmt::print("Config:\n{}\n", cfg.str());

    return cfg;
}

namespace {

std::string fmtDPort(bool wildcard, uint16_t dport) {
    if (wildcard) return "*";
    return fmt::format("{}", dport);
}

std::string fmtSource(net::IPv4Address source) {
    if (source == net::IPv4Address{})  return "*";
    return fmt::format("{}", source);
}

std::string fmtSender(bool sender, unsigned ttl) {
    if (! sender) return "NO";
    return fmt::format("YES, TTL = {}", ttl);
}

std::string fmtCount(uint64_t count) {
    if (count == 0) return "unlimited";
    return fmt::format("{}", count);
}

} // anon.namespace

std::string Config::str() const {
    ParamDescrList params{
        formatParam("Group", group_),
        formatParam("UDP port", fmtDPort(wildcard_, dport_)),
        formatParam("Interface", intf_),
        formatParam("Interface IP address", intfAddr_),
        formatParam("Source", fmtSource(source_)),
        formatParam("Sender", fmtSender(sender_, ttl_)),
        formatParam("Count", fmtCount(count_)),
        formatParam("Show payload", showPayload_ ? "YES" : "NO"),
        formatParam("Colors", colors_ ? "YES" : "NO")
    };

    return formatParams(params);
}

} // namespace malt
