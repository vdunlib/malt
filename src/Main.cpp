#include "vdunlib/unix/SignalHandler.hpp"

#include "Config.hpp"
#include "OutputHandler.hpp"
#include "Malt.hpp"

namespace malt {
namespace {
bool stopped{false};
} // anon.namespace
} // namespace malt

int main(int argc, char const* const* argv) {
    auto cfg = malt::Config::forArgs(argc, argv);
    malt::OutputHandler oh{cfg};

    vdunlib::SignalHandler<>::install
    <SIGINT, SIGTERM, SIGHUP>([] (int) { malt::stopped = true; });

    auto mr = malt::makeRunner(cfg, oh, malt::stopped);

    if (! mr->run()) return 1;

    return 0;
}