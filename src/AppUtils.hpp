#pragma once

#include <cstdint>
#include <cstdlib>
#include <string>

#include <fmt/format.h>

#include "vdunlib/core/CompilerUtils.hpp"
#include "vdunlib/unix/SysError.hpp"
#include "vdunlib/text/Joiner.hpp"

using namespace vdunlib;

namespace malt {

std::string strTs(uint64_t);

template <typename ... Ts>
void appAbort(Ts&& ... args) {
    fmt::print(stderr, "error: {}\n",
            text::str_join(std::forward<Ts>(args)...));
    exit(1);
}

template <typename ... Ts>
bool error(Ts&& ... args) {
    fmt::print(stderr, "error: {}\n",
                text::str_join(std::forward<Ts>(args)...));
    return false;
}

template <typename ... Ts>
void warning(Ts&& ... args) {
    fmt::print(stderr, "warning: {}\n",
                text::str_join(std::forward<Ts>(args)...));
}

template <typename ... Ts>
void warningTs(uint64_t ts, Ts&& ... args) {
    fmt::print(stderr, "{}, warning: {}\n", strTs(ts),
                text::str_join(std::forward<Ts>(args)...));
}

VDUNLIB_ALWAYS_INLINE
bool sysCallError(std::string const& msg) {
    fmt::print(stderr, "error: {}: {}\n", msg, sysError(errno));
    return false;
}

} // namespace malt
