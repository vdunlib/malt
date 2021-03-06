#pragma once

#include <cstdint>

#include "vdunlib/core/CompilerUtils.hpp"

namespace vdunlib {
struct NanosText final {
    char buf[10];
    uint64_t carry;

    VDUNLIB_ALWAYS_INLINE
    uint64_t prc(uint64_t nanos, unsigned prec) {
        carry = 0;
        if (prec == 0 || nanos == 0) buf[0] = '\0';
        else {
            int ln0{0};
            bool ln0set{false};
            for (int i = 8; i > -1; --i) {
                uint64_t lastD = nanos % 10 + carry;
                nanos /= 10;
                if (lastD == 10) lastD = 0;
                else carry = 0;

                if (static_cast<unsigned>(i) < prec) {
                    if (! ln0set) {
                        if (lastD == 0) continue;

                        ln0set = true;
                        ln0 = i;
                    }
                    buf[i] = '0' + lastD;
                } else {
                    if (static_cast<unsigned>(i) > prec) continue;
                    else carry = lastD > 4 ? 1 : 0;
                }
            }
            if (! ln0set) buf[0] = '\0';
            else buf[ln0 + 1] = '\0';
        }
        return carry;
    }
};

} // namespace vdunlib