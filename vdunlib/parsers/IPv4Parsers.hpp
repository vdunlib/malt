#pragma once

#include <tuple>
#include <string>

#include "vdunlib/net/IPv4Address.hpp"
#include "vdunlib/net/IPv4Prefix.hpp"

namespace vdunlib {

template <typename IPv4Object>
std::tuple<IPv4Object, bool> parse(std::string const &);

template <>
std::tuple<net::IPv4Address, bool> parse<net::IPv4Address>(std::string const&);

template <>
std::tuple<net::IPv4Prefix, bool> parse<net::IPv4Prefix>(std::string const&);

template <typename IPv4Object, typename OnError>
IPv4Object parse(std::string const& s, OnError&& onError) {
    auto pr = parse<IPv4Object>(s);
    if (! std::get<bool>(pr)) {
        onError();
        return IPv4Object{};
    }

    return std::get<IPv4Object>(pr);
}

template <typename IPv4Object>
std::tuple<IPv4Object, bool> parse(char const*);

template <>
std::tuple<net::IPv4Address, bool> parse<net::IPv4Address>(char const*);

template <>
std::tuple<net::IPv4Prefix, bool> parse<net::IPv4Prefix>(char const*);

template <typename IPv4Object, typename OnError>
IPv4Object parse(char const* s, OnError&& onError) {
    auto pr = parse<IPv4Object>(s);
    if (! std::get<bool>(pr)) {
        onError();
        return IPv4Object{};
    }

    return std::get<IPv4Object>(pr);
}

} // namespace vdunlib