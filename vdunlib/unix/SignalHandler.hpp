#pragma once

#include <csignal>
#include <cstring>

#include <fmt/format.h>

namespace vdunlib {

struct AbortPolicy {
    AbortPolicy() = delete;

    static inline void fail(int signal) {
        fmt::print(stderr,
                "error: unable to install handler for {}: {}\n",
                strsignal(signal), strerror(errno));
        exit(1);
    }
};

/**
 * A utility class which helps install the same handler for one
 * or multiple UNIX signals.
 *
 * @tparam ExitPolicy The class that provides a static method
 * `void fail(int signal)` which is invoked if the handler fails
 * to be installed for a specific signal, which is passed as the
 * only argument to this static method.
 */
template <typename ExitPolicy = AbortPolicy>
class SignalHandler final {
public:
    SignalHandler() = delete;

    /**
     * Installs the specified signal handler `h` for the signals
     * specified as the template arguments to this function.
     * @tparam Signals the signals for which this signal hander
     * is installed
     * @param h the signal handler
     * @return `true` if the signal handler is successfully installed
     * for each of the signals, `false` otherwise
     */
    template <int ... Signals>
    static bool install(void (*h)(int)) {
        struct sigaction sa;
        sigemptyset(&sa.sa_mask);
        sa.sa_flags = 0;
        sa.sa_handler = h;
        return installImpl<Signals...>(&sa);
    }
private:
    template <int Signal>
    static bool installImpl(struct sigaction* sa) {
        if (sigaction(Signal, sa, nullptr) == -1) {
            ExitPolicy::fail(Signal);
            return false;
        }
        return true;
    }

    template <int First, int Second, int ... Rest>
    static bool installImpl(struct sigaction* sa) {
        if (sigaction(First, sa, nullptr) == -1) {
            ExitPolicy::fail(First);
            return false;
        }
        return installImpl<Second, Rest...>(sa);
    }
};

} // namespace vdunlib
