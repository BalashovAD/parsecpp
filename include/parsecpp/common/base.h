#pragma once

#include <parsecpp/core/parser.h>
#include <parsecpp/utils/cmp.h>

namespace prs {


template <typename T>
auto pure(T t) noexcept {
    return Parser<T>::make([t](Stream&) {
        return Parser<T>::data(t);
    });
}


inline auto success() noexcept {
    return pure(Unit{});
}


inline auto fail() noexcept {
    return Parser<Unit>::make([](Stream& s) {
        return Parser<Unit>::makeError("Fail", s.pos());
    });
}


inline auto fail(std::string text) noexcept {
    return Parser<Unit>::make([text](Stream& s) {
        return Parser<Unit>::makeError(text, s.pos());
    });
}

}