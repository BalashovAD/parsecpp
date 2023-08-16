#pragma once

#include <parsecpp/core/parser.h>
#include <parsecpp/common/string.h>

#include <charconv>


namespace prs {

namespace {

template <typename Number, typename = std::void_t<>>
constexpr bool hasFromCharsMethod = false;

template <typename Number>
constexpr bool hasFromCharsMethod<Number, std::void_t<decltype(std::from_chars(nullptr, nullptr, std::declval<Number&>()))>> = true;

}

/**
 * @return Parser<Number>
 */
template <typename Number = double>
    requires (std::is_arithmetic_v<Number>)
auto number() noexcept {
    return Parser<Number>::make([](Stream& s) {
        auto sv = s.sv();
        Number n{};
        if constexpr (hasFromCharsMethod<Number>) {
            if (auto res = std::from_chars(sv.data(), sv.data() + sv.size(), n);
                    res.ec == std::errc{}) {
                s.moveUnsafe(res.ptr - sv.data());
                return Parser<Number>::data(n);
            } else {
                // TODO: get error and pos from res
                return Parser<Number>::makeError("Cannot parse number", s.pos());
            }
        } else { // some system doesn't have from_chars for floating point numbers
            if constexpr (std::is_same_v<Number, double>) {
                char *end = const_cast<char*>(sv.end());
                n = std::strtod(sv.data(), &end);
                size_t endIndex = end - sv.data();
                if (endIndex == 0) {
                    return Parser<Number>::makeError("Cannot parse number", s.pos());
                }
                if (errno == ERANGE) {
                    return Parser<Number>::makeError("Cannot parse number ERANGE", s.pos());
                }
                s.moveUnsafe(endIndex);
                return Parser<Number>::data(n);
            } else if (std::is_same_v<Number, float>) {
                char *end = const_cast<char*>(sv.end());
                n = std::strtof(sv.data(), &end);
                size_t endIndex = end - sv.data();
                if (endIndex == 0) {
                    return Parser<Number>::makeError("Cannot parse number", s.pos());
                }
                if (errno == ERANGE) {
                    return Parser<Number>::makeError("Cannot parse number ERANGE", s.pos());
                }
                s.moveUnsafe(endIndex);
                return Parser<Number>::data(n);
            } else {
                static_assert(!std::is_void_v<Number>, "Number type doesn't support");
                return Parser<Number>::makeError("Not supported", s.pos());
            }
        }
    });
}


/**
 *
 * @return Parser<char>
 */
constexpr auto digit() noexcept {
    return charFrom<FromRange('0', '9')>();
}


inline auto digits() noexcept {
    return lettersFrom<FromRange('0', '9')>();
}

}