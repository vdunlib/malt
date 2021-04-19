#pragma once

#include <fmt/format.h>

#include "vdunlib/net/IPv4Address.hpp"
#include "vdunlib/net/IPv4Prefix.hpp"

namespace fmt {

template<>
struct formatter<vdunlib::net::IPv4Address> : formatter<string_view> {

    template<typename FormatContext>
    auto format(const vdunlib::net::IPv4Address& addr, FormatContext& ctx) {
        return format_to(
                ctx.out(),
                "{}.{}.{}.{}",
                addr.oct1(), addr.oct2(), addr.oct3(), addr.oct4());
    }
};

template<>
struct formatter<vdunlib::net::IPv4Prefix> : formatter<string_view> {

    template<typename FormatContext>
    auto format(const vdunlib::net::IPv4Prefix& pfx, FormatContext& ctx) {
        return format_to(ctx.out(), "{}/{}", pfx.address(), pfx.length());
    }
};

} // namespace fmt