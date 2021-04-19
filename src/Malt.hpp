#pragma once

#include <memory>
#include "Config.hpp"
#include "OutputHandler.hpp"

namespace malt {

struct IMaltRunner {
    virtual bool run() = 0;
    virtual ~IMaltRunner() = default;
};

/**
 * Creates a malt runner based on the specified config.
 *
 * @return a unique ptr of the runner
 */
std::unique_ptr<IMaltRunner> makeRunner(Config const&, OutputHandler&, bool&);


} // namespace malt