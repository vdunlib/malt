#pragma once

#include <unistd.h>

#include "Config.hpp"
#include "OutputHandler.hpp"

namespace malt {

class MaltBase {
protected:
    Config const& cfg_;
    OutputHandler& oh_;
    int s_;
    bool& stopped_;

    MaltBase(Config const& cfg, OutputHandler& oh, bool& stopped)
    : cfg_{cfg}, oh_{oh}, s_{-1}, stopped_{stopped} {}

    ~MaltBase() {
        if (s_ != -1) {
            int rc;
            do {
                rc = close(s_);
            } while (rc == -1 && errno == EINTR);
        }
    }
};

} // namespace malt