#pragma once

#include <cstdint>
#include <ctime>
#include <utility>
#include <chrono>
#include <string>

#include "vdunlib/core/CompilerUtils.hpp"

namespace vdunlib {

constexpr uint64_t NanosInSecond{1'000'000'000ul};
constexpr uint64_t NanosInDay{24ul * 3'600ul * NanosInSecond};

/**
 * A utility class which allows conversion of series of chrono
 * literals to uint64_t value representing time.
 */
class Nanos final {
public:
    /**
     * Constructs a Nanos object with the specified timepoint
     * in nanoseconds.
     *
     * @param base the base time in nanoseconds
     */
    constexpr explicit Nanos(uint64_t base): base_{base} {}

    /**
     * Returns the result of the sum of the specified arguments each
     * converted to nanoseconds added to the base time.
     *
     * @tparam Duration the argument types convertible to chrono duration
     * @param ds the chrono duration arguments typically expressed as
     * chrono duration literals
     * @return the sum of the arguments converted to nanoseconds added to
     * the base time
     */
    template <typename ... Duration>
    constexpr uint64_t add(Duration&& ... ds) const {
        return sumImpl(std::forward<Duration>(ds)...) + base_;
    }

    /**
     * Returns the nanosecond value of a duration computed as a sum of the
     * parameters passed to this function
     *
     * @tparam Duration the argument types convertible to chrono duration
     * @param ds the chrono duration arguments typically expressed as
     * chrono duration literals
     * @return the nanosecond value of a duration computed as a sum of the
     * parameters passed to this function
     */
    template <typename ... Duration>
    static inline constexpr uint64_t dur(Duration&& ... ds) {
        return sumImpl(std::forward<Duration>(ds)...);
    }

private:
    uint64_t base_;

    template <typename Duration>
    static inline constexpr uint64_t count(Duration&& d) {
        return static_cast<uint64_t>(
                std::chrono::duration_cast<std::chrono::nanoseconds>(
                        std::forward<Duration>(d)).count());
    }

    static inline constexpr uint64_t sumImpl() { return 0; }

    template <typename First, typename ... Rest>
    static inline constexpr uint64_t sumImpl(First&& d, Rest&& ...ds) {
        return count(std::forward<First>(d)) + sumImpl(std::forward<Rest>(ds)...);
    }
};

/**
 * An object representing a duration in nanoseconds
 */
class Duration final {
    friend class Timepoint;
public:

    static inline constexpr Duration nanos(uint64_t d) {
        return Duration{d};
    }

    template <typename ... Ds>
    static inline constexpr Duration of(Ds&& ... ds) {
        return Duration{Nanos::dur(std::forward<Ds>(ds)...)};
    }

    constexpr uint64_t in_nanos() const { return d_; }

    constexpr Duration operator+ (Duration rhs) const {
        return Duration{d_ + rhs.d_};
    }

    constexpr Duration operator- (Duration rhs) const {
        return Duration{d_ - rhs.d_};
    }

    constexpr Duration operator* (uint64_t m) const {
        return Duration{d_ * m};
    }

    constexpr bool operator== (Duration rhs) const {
        return d_ == rhs.d_;
    }

    constexpr bool operator!= (Duration rhs) const {
        return d_ != rhs.d_;
    }

    constexpr bool operator< (Duration rhs) const {
        return d_ < rhs.d_;
    }

    constexpr bool operator<= (Duration rhs) const {
        return d_ <= rhs.d_;
    }

    constexpr bool operator> (Duration rhs) const {
        return d_ > rhs.d_;
    }

    constexpr bool operator>= (Duration rhs) const {
        return d_ >= rhs.d_;
    }

    constexpr operator bool() const {
        return d_ != 0;
    }

private:
    uint64_t d_;

    constexpr explicit Duration(uint64_t d): d_{d} {}
};

std::string to_str(Duration d);

/**
 * An object representing a time point expressed in nanoseconds from
 * the epoch.
 */
class Timepoint final {
public:

    static inline constexpr Timepoint nanos(uint64_t ts) {
        return Timepoint{ts};
    }

    constexpr uint64_t in_nanos() const { return t_; }

    constexpr Timepoint operator+ (Duration d) const {
        return Timepoint{t_ + d.d_};
    }

    constexpr Timepoint operator- (Duration d) const {
        return Timepoint{t_ - d.d_};
    }

    constexpr bool operator== (Timepoint rhs) const {
        return t_ == rhs.t_;
    }

    constexpr bool operator!= (Timepoint rhs) const {
        return t_ != rhs.t_;
    }

    constexpr bool operator< (Timepoint rhs) const {
        return t_ < rhs.t_;
    }

    constexpr bool operator<= (Timepoint rhs) const {
        return t_ <= rhs.t_;
    }

    constexpr bool operator> (Timepoint rhs) const {
        return t_ > rhs.t_;
    }

    constexpr bool operator>= (Timepoint rhs) const {
        return t_ >= rhs.t_;
    }

private:
    uint64_t t_;

    constexpr explicit Timepoint(uint64_t t): t_{t} {}
};

/**
 * Creates a textual representation of the timepoint as the host's local time.
 *
 * @param tp
 * @return
 */
std::string to_local_str(Timepoint tp);

class TimeUtils final {
public:
    TimeUtils() = delete;

    VDUNLIB_ALWAYS_INLINE
    static uint64_t gethostnanos() {
        timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);
        return static_cast<uint64_t>(ts.tv_sec) * NanosInSecond + ts.tv_nsec;
    }
};

} // namespace vdunlib
