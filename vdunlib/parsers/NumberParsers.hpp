#pragma once

#include <cstdint>
#include <tuple>
#include <string>

namespace vdunlib {

enum class UIntParseStatus: unsigned {
    Success = 0,   // uint was parsed successfully,
    Invalid = 1,   // the input string contains non-digit characters
    Overflow = 2   // the value represented by the string is larger
                   // than the maximum uint64_t value
};

using UIntParseResult = std::tuple<uint64_t, UIntParseStatus>;

UIntParseResult parseUInt64(const std::string&);

template <typename OnInvalid, typename OnOverflow>
uint64_t parseUInt64(const std::string& s,
        OnInvalid&& onInvalid, OnOverflow&& onOverflow) {
    auto pr = parseUInt64(s);
    if (std::get<UIntParseStatus>(pr) == UIntParseStatus::Success)
        return std::get<uint64_t>(pr);

    if (std::get<UIntParseStatus>(pr) == UIntParseStatus::Overflow)
        onOverflow();
    else onInvalid();
    return 0ul;
}

} // namespace vdunlib
