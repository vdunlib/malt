#include <sys/socket.h>
#include <netinet/in.h>
#include <ifaddrs.h>

#include "vdunlib/unix/SysError.hpp"

#include "AppUtils.hpp"
#include "IPv4IntfList.hpp"

namespace malt {

std::vector<std::tuple<std::string, net::IPv4Address>> getIPv4IntfList() {
    ifaddrs *intfs;

    if (getifaddrs(&intfs) == -1) {
        appAbort("cannot get host interace list: ", sysError(errno));
    }

    std::vector<std::tuple<std::string, net::IPv4Address>> intfList;
    for (auto p = intfs; p != nullptr; p = p->ifa_next) {
        if (p->ifa_addr == nullptr) continue;

        if (p->ifa_addr->sa_family == AF_INET) {
            sockaddr_in *sin = reinterpret_cast<sockaddr_in *>(p->ifa_addr);
            intfList.emplace_back(std::make_tuple(
                    std::string{p->ifa_name},
                    net::IPv4Address::from_nl(sin->sin_addr.s_addr)));
        }
    }

    freeifaddrs(intfs);
    return intfList;
}

} // namespace malt