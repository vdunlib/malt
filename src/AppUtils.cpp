#include <unistd.h>
#include <cstdint>
#include <csignal>
#include <ctime>
#include <string>

#include <fmt/format.h>
#include <fmt/chrono.h>

#include "vdunlib/formatters/NanosText.hpp"

using namespace vdunlib;

namespace malt {

std::string strTs(uint64_t ts) {
    time_t epochTime = ts / 1'000'000'000ul;
    tm tms{};
    uint64_t nanos = ts % 1'000'000'000ul;
    NanosText nt;
    epochTime += nt.prc(nanos, 3);

    return fmt::format("{:%H:%M:%S}.{}", *localtime_r(&epochTime, &tms), nt.buf);
}

} // namespace malt