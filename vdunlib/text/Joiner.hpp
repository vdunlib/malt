#pragma once

#include <string>

#include <fmt/format.h>

namespace vdunlib {
namespace text {  

template <typename Buffer = fmt::memory_buffer>
class TextJoiner final {
public:
    TextJoiner(): sep_{} {}

    TextJoiner(const TextJoiner&) = delete;
    TextJoiner(TextJoiner&&) noexcept = default;
    TextJoiner& operator= (const TextJoiner&) = delete;
    TextJoiner& operator= (TextJoiner&&) noexcept = default;
    ~TextJoiner() = default;

    static inline TextJoiner on(std::string sep) {
        return TextJoiner{{}, {}, std::move(sep)};
    }

    static inline TextJoiner with(
            std::string prologue, std::string epilogue, std::string sep) {
        return TextJoiner{std::move(prologue), std::move(epilogue), std::move(sep)};
    }

    template <typename ... Ts>
    TextJoiner& append(Ts&& ... args) {
        if (sizeof...(Ts) == 0) return *this;

        adjoin(std::forward<Ts>(args)...);
        return *this;
    }

    std::string str() {
        if (buf_.size() == 0) {
            if (prologue_.empty() && epilogue_.empty())
                return {};
            if (prologue_.empty()) return epilogue_;
            if (epilogue_.empty()) return prologue_;

            return fmt::format("{}{}", prologue_, epilogue_);
        }

        fmt::format_to(buf_, "{}", epilogue_);
        auto result = fmt::to_string(buf_);
        buf_.clear();

        return result;
    }

    template <typename ... Ts>
    std::string join(Ts&& ... args) {
        append(std::forward<Ts>(args)...);
        return str();
    }

private:
    Buffer buf_;
    std::string prologue_;
    std::string epilogue_;
    std::string sep_;

    explicit TextJoiner(
            std::string prologue,
            std::string epilogue,
            std::string sep)
    : prologue_{std::move(prologue)}
    , epilogue_{std::move(epilogue)}
    , sep_{std::move(sep)} {}

    void adjoin() {}

    template <typename T, typename ...Ts>
    void adjoin(T&& first, Ts&& ... rest) {
        if (buf_.size() == 0) {
            fmt::format_to(buf_, "{}", prologue_);
        }
        else fmt::format_to(buf_, "{}", sep_);
        fmt::format_to(buf_, "{}", std::forward<T>(first));
        append(std::forward<Ts>(rest)...);
    }
};

using Joiner = TextJoiner<>;

template <typename ... Ts>
std::string str_join(Ts&& ... args) {
    return TextJoiner<>{}.join(std::forward<Ts>(args)...);
}

} // namespace text  
} // namespace vdunlib
