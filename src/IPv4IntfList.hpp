#pragma once

#include <tuple>
#include <vector>
#include <string>

#include "vdunlib/net/IPv4Address.hpp"

namespace malt {

std::vector<std::tuple<std::string, net::IPv4Address>> getIPv4IntfList();

} // namespace malt