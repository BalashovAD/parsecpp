#pragma once

#include <concepts>
#include <utility>


namespace prs::details {

template <typename T, typename First, typename ...Args>
bool cmpAnyOf(T const& t, First &&f, Args &&...args) noexcept {
    if constexpr (sizeof...(args) > 0) {
        return f == t || cmpAnyOf(t, std::forward<Args>(args)...);
    } else {
        return f == t;
    }
}

}