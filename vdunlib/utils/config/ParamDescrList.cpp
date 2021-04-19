#include <tuple>
#include <vector>
#include <string>
#include <algorithm>

#include <fmt/format.h>

#include "vdunlib/utils/config/ParamDescrList.hpp"

namespace vdunlib {

namespace {
std::string repC(char c, unsigned r) {
    if (r == 0) return {};
    char buf[r + 1];
    for (unsigned i = 0; i < r; i++) buf[i] = c;
    buf[r] = 0;
    return buf;
}
} // anon.namespace

std::string formatParams(const ParamDescrList& params) {
    auto ffw = std::get<0>(*std::max_element(
            params.cbegin(), params.cend(),
            [](const auto& a, const auto& b) {
                return std::get<0>(a).length() < std::get<0>(b).length();
            })).length();
    fmt::memory_buffer buf;
    std::for_each(
            params.cbegin(), params.cend(),
            [ffw, &buf](const auto& e) {
                const auto& f1 = std::get<0>(e);
                const auto& f2 = std::get<1>(e);
                fmt::format_to(
                        buf,
                        "  {}: {}{}\n",
                        f1, repC(' ', ffw - f1.length()), f2);
            });
    return fmt::to_string(buf);
}

} // namespace vdunlib
