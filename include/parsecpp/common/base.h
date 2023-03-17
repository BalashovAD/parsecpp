#pragma once

#include <parsecpp/core/parser.h>
#include <parsecpp/utils/cmp.h>

namespace prs {

/**
 *
 * @return Parser<T>
 */
template <typename T>
auto pure(T tValue) noexcept {
    using Value = std::decay_t<T>;
    return Parser<Value>::make([t = std::move(tValue)](Stream&) {
        return Parser<Value>::data(t);
    });
}

/**
 *
 * @return Parser<Unit>
 */
inline auto success() noexcept {
    return pure(Unit{});
}

/**
 *
 * @return Parser<Unit>
 */
inline auto fail() noexcept {
    return Parser<Unit>::make([](Stream& s) {
        return Parser<Unit>::makeError("Fail", s.pos());
    });
}

/**
 *
 * @return Parser<Unit>
 */
inline auto fail(std::string const& text) noexcept {
    return Parser<Unit>::make([text](Stream& s) {
        return Parser<Unit>::makeError(text, s.pos());
    });
}

}