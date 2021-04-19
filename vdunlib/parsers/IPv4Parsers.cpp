#include <tuple>
#include <cctype>

#include "vdunlib/core/CompilerUtils.hpp"
#include "vdunlib/parsers/IPv4Parsers.hpp"

namespace vdunlib {

namespace {

VDUNLIB_ALWAYS_INLINE
bool isDig(char c) {
    return std::isdigit(static_cast<unsigned char>(c));
}

VDUNLIB_ALWAYS_INLINE
constexpr unsigned dVal(char digit_char) {
    return static_cast<unsigned>(digit_char - '0');
}

template <typename ChrItr>
static constexpr decltype(auto) parseFailed(ChrItr start) {
    return std::make_tuple(static_cast<uint32_t>(0), start);
}

template<uint32_t MaxVal, typename ChrT, typename ChrItr, typename Pred>
decltype(auto) parseBoundedUInt(ChrItr start, Pred&& isEnd) {
    uint32_t oct{0};
    ChrItr ii{start};
    bool haveDigs{false};
    while (!isEnd(ii)) {
        ChrT c = *ii;
        if (isDig(c)) {
            oct = oct * 10 + dVal(c);
            if (oct > MaxVal)
                return parseFailed(start);
            haveDigs = true;
            ++ii;
        } else break;
    }

    if (! haveDigs) return parseFailed(start);
    return std::make_tuple(oct, ii);
}

/**
 * Parses an IPv4 address in the dotted decimal notation at the beginning
 * of the string passed as the character iterator. If successful returns a
 * tuple containing a 32-bit unsigned integer equal to the value of the
 * IP address and either an iterator pointing to the next character right
 * after the address or an iterator pointing to the start of the string,
 * otherwise returns a tuple containing 0 and an iterator pointing the start
 * of the string.
 *
 * @tparam CharT The character type
 * @tparam ChrItr the character iterator
 * @tparam Predicate a callable with the signature `(ChrItr) -> bool`
 * @param start an iterator pointing to the beginning of the string
 * @param isEnd a predicate which takes an iterator and returns true of
 * the iterator points to the end of the string, or false otherwise
 * @return a tuple consisting of the parsed unsigned 32-bit value and an
 * iterator pointing to the character following the address, or an iterator
 * pointing to the end of the string. If parsing fails the returned tuple
 * contains 0 and an iterator pointing to the start of the string.
 */
template<typename ChrT, typename ChrItr, typename Pred>
decltype(auto) parseIPv4Address(ChrItr start, Pred&& isEnd) {
    uint32_t o1;
    ChrItr octStart{start}, octEnd;
    std::tie(o1, octEnd) =
            parseBoundedUInt<255, ChrT>(octStart, std::forward<Pred>(isEnd));
    if (octEnd == octStart ||
        isEnd(octEnd) ||
        *octEnd != '.') return parseFailed(start);

    uint32_t o2;
    octStart = octEnd + 1;
    std::tie(o2, octEnd) =
            parseBoundedUInt<255, ChrT>(octStart, std::forward<Pred>(isEnd));
    if (octEnd == octStart ||
        isEnd(octEnd) ||
        *octEnd != '.') return parseFailed(start);

    uint32_t o3;
    octStart = octEnd + 1;
    std::tie(o3, octEnd) =
            parseBoundedUInt<255, ChrT>(octStart, std::forward<Pred>(isEnd));
    if (octEnd == octStart ||
        isEnd(octEnd) ||
        *octEnd != '.') return parseFailed(start);

    uint32_t o4;
    octStart = octEnd + 1;
    std::tie(o4, octEnd) =
            parseBoundedUInt<255, ChrT>(octStart, std::forward<Pred>(isEnd));
    if (octEnd == octStart) return parseFailed(start);

    return std::make_tuple((o1 << 24u) + (o2 << 16u) + (o3 << 8u) + o4, octEnd);
}

/**
 * Parses an IPv4 prefix length in the bitcount notation at the beginning
 * of the string passed as the character iterator. If successful returns a
 * tuple containing the prefix length as a 32-bit unsigned integer and either
 * an iterator pointing to the next character right after the prefix length
 * or or an iterator pointing to the end of string, otherwise returns
 * a tuple containing 0 and an iterator pointing the start of the string.
 *
 * @tparam CharT The character type
 * @tparam ChrItr the character iterator
 * @tparam Predicate a callable with the signature `(ChrItr) -> bool`
 * @param start an iterator pointing to the beginning of the string
 * @param isEnd a predicate which takes an iterator and returns true of
 * the iterator points to the end of the string, or false otherwise
 * @return a tuple containing the prefix length as a 32-bit unsigned integer
 * and either an iterator pointing to the next character right after the
 * prefix length or or an iterator pointing to the end of string. If parsing
 * fails returns a tuple containing 0 and an iterator pointing the start of
 * the string.
 */
template<typename ChrT, typename ChrItr, typename Pred>
decltype(auto) parseIPv4PrefLen(ChrItr start, Pred&& isEnd) {
    ChrItr pls{start};

    if (isEnd(pls))
        return parseFailed(start);

    if (*pls++ != '/')
        return parseFailed(start);

    uint32_t plen;
    ChrItr ple;
    std::tie(plen, ple) =
            parseBoundedUInt<32, ChrT>(pls, std::forward<Pred>(isEnd));
    if (ple == pls) return parseFailed(start);

    return std::make_tuple(plen, ple);
}

/**
 * Parses an IPv4 prefix in the dotted decimal notation with prefix length
 * expressed in the bitcount notation at the beginning of the string passed
 * as the character iterator. If successful returns a tuple containing the
 * prefix and an iterator pointing to the next character right after the
 * prefix or the iterator at the string end, otherwise returns a tuple
 * containing the default prefix (0.0.0.0/0) and the start of string iterator.
 *
 *
 * @tparam CharT The character type
 * @tparam ChrItr the character iterator
 * @tparam Predicate a callable with the signature `(ChrItr) -> bool`
 * @param start an iterator pointing to the beginning of the string
 * @param isEnd a predicate which takes an iterator and returns true of
 * the iterator points to the end of the string, or false otherwise
 * @return a tuple containing the prefix and an iterator pointing to the
 * next character right after the prefix or an iterator pointing to the
 * end of the string. If parsing fails returns a tuple containing the
 * default prefix (0.0.0.0/0) and an iterator pointing to the start of
 * the string.
 */
template<typename ChrT, typename ChrItr, typename Pred>
decltype(auto) parseIPv4Prefix(ChrItr start, Pred&& isEnd) {
    uint32_t addr;
    ChrItr pfs{start}, pfe;
    std::tie(addr, pfe) =
            parseIPv4Address<ChrT>(pfs, std::forward<Pred>(isEnd));
    if (pfe == pfs || isEnd(pfe))
        return std::make_tuple(net::IPv4Prefix{}, start);

    uint32_t plen;
    pfs = pfe;
    std::tie(plen, pfe) =
            parseIPv4PrefLen<ChrT>(pfs, std::forward<Pred>(isEnd));
    if (pfe == pfs)
        return std::make_tuple(net::IPv4Prefix{}, start);

    return std::make_tuple(
            net::IPv4Prefix::make(net::IPv4Address{addr}, plen), pfe);
}
} // anon.namespace

template <>
std::tuple<net::IPv4Address, bool> parse<net::IPv4Address>(std::string const& s) {
    uint32_t addr;
    std::string::const_iterator ae;
    std::tie(addr, ae) = parseIPv4Address<char>(
            s.cbegin(), [&s] (auto ii) { return ii == s.cend(); });
    if (ae == s.cbegin() || ae != s.cend())
        return std::make_tuple(net::IPv4Address{}, false);
    return std::make_tuple(net::IPv4Address{addr}, true);
}

template <>
std::tuple<net::IPv4Address, bool> parse<net::IPv4Address>(char const* s) {
    uint32_t addr;
    char const* ae;
    std::tie(addr, ae) = parseIPv4Address<char>(
            s, [] (auto const *p) { return *p == '\0'; });
    if (ae == s || *ae != '\0')
        return std::make_tuple(net::IPv4Address{}, false);
    return std::make_tuple(net::IPv4Address{addr}, true);
}

template <>
std::tuple<net::IPv4Prefix, bool> parse<net::IPv4Prefix>(std::string const& s) {
    auto r = parseIPv4Prefix<char>(
            s.cbegin(), [&s] (auto ii) { return ii == s.cend(); });
    auto ii = std::get<std::string::const_iterator>(r);
    if (ii == s.cbegin() || ii != s.cend())
        return std::make_tuple(net::IPv4Prefix{}, false);
    return std::make_tuple(std::get<net::IPv4Prefix>(r), true);
}

template <>
std::tuple<net::IPv4Prefix, bool> parse<net::IPv4Prefix>(char const* s) {
    auto r = parseIPv4Prefix<char>(
            s, [] (auto const *p) { return *p == '\0'; });
    auto ae = std::get<char const*>(r);
    if (ae == s || *ae != '\0')
        return std::make_tuple(net::IPv4Prefix{}, false);
    return std::make_tuple(std::get<net::IPv4Prefix>(r), true);
}


} // namespace vdunlib
