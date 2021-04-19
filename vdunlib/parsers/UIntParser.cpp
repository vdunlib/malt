#include <cstdint>
#include <cctype>
#include <tuple>
#include <string>

#include "vdunlib/parsers/NumberParsers.hpp"

namespace vdunlib {

namespace {
// max uint64_t in decimal: 18446744073709551615
constexpr uint64_t LastStep{1844674407370955161ul};

constexpr UIntParseResult invalid() {
    return std::make_tuple(0ul, UIntParseStatus::Invalid);
}

constexpr UIntParseResult overflow() {
    return std::make_tuple(0ul, UIntParseStatus::Overflow);
}
} // anon.namespace

UIntParseResult parseUInt64(const std::string& s) {
    if (s.empty()) return invalid();

    bool ovf{false};
    uint64_t acc{0};

    for (char c: s) {
        if (! std::isdigit(static_cast<unsigned char>(c)))
            return invalid();

        if (ovf) continue;

        auto d = static_cast<uint64_t>(c - '0');
        if (acc < LastStep || (acc == LastStep && d < 6)) {
            acc = acc * 10 + d;
            continue;
        }

        ovf = true;
    }

    if (ovf) return overflow();
    return std::make_tuple(acc, UIntParseStatus::Success);
}

} // namespace vdunlib:
