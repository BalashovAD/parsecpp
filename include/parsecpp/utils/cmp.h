#pragma once

#include <concepts>
#include <utility>


namespace prs::details {

template <typename T, std::equality_comparable_with<T> First, typename ...Args>
bool cmpAnyOf(T const& t, First &&f, Args &&...args) noexcept {
    if constexpr (sizeof...(args) > 0) {
        return t == f || cmpAnyOf(t, std::forward<Args>(args)...);
    } else {
        return t == f;
    }
}

}