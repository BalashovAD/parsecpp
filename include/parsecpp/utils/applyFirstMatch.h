#pragma once

#include <functional>
#include <tuple>

namespace prs::details {

template <typename UnhandledAction, typename Equal, typename ...TupleArgs>
class ApplyFirstMatch {
public:
    explicit constexpr ApplyFirstMatch(UnhandledAction unhandled, Equal eq, TupleArgs &&...args) noexcept
        : m_tuple(std::make_tuple(std::forward<TupleArgs>(args)...))
        , m_unhandled(std::move(unhandled))
        , m_cmp(std::move(eq)) {
    }

    template <typename KeyLike, typename ...Args>
    constexpr decltype(auto) apply(KeyLike const& key, Args &&...args) const {
        auto f = [&](auto const& el) -> bool {
            return std::invoke(m_cmp, el, key);
        };

        using ResultType = decltype(invoke(m_unhandled, args...));
        return foreach<ResultType, 0>(f, std::forward<Args>(args)...);
    }

    static constexpr size_t size() noexcept {
        return sizeof ...(TupleArgs);
    }
private:
    template <typename F, typename ...Args>
    constexpr decltype(auto) invoke(F const& f, Args &&...args) const {
        if constexpr (std::is_invocable_v<F, Args...>) {
            return std::invoke(f, std::forward<Args>(args)...);
        } else {
            static_assert(sizeof...(Args) == 0);
            return f;
        }
    }

    template <typename ResultType, unsigned shift, typename ...Args>
    constexpr ResultType foreach(auto const& checkFn, Args &&...args) const {
        auto const& [key, fn] = get<shift>(m_tuple);
        if (checkFn(key)) {
            return invoke(fn, std::forward<Args>(args)...);
        } else {
            if constexpr (std::tuple_size_v<decltype(m_tuple)> > shift + 1) {
                return foreach<ResultType, shift + 1>(checkFn, args...);
            } else {
                return invoke(m_unhandled, std::forward<Args>(args)...);
            }
        }
    }

    std::tuple<TupleArgs...> const m_tuple;
    UnhandledAction const m_unhandled;
    Equal const m_cmp;
};


template <typename UnhandledAction, typename ...TupleArgs>
static constexpr auto makeFirstMatch(UnhandledAction unhandled, TupleArgs &&...args) noexcept {
    return ApplyFirstMatch(std::move(unhandled), std::equal_to<>{}, std::forward<TupleArgs>(args)...);
}

template <typename UnhandledAction, typename Equal, typename ...TupleArgs>
static constexpr auto makeFirstMatchEq(UnhandledAction unhandled, Equal eq, TupleArgs &&...args) noexcept {
    return ApplyFirstMatch(std::move(unhandled), std::move(eq), std::forward<TupleArgs>(args)...);
}

}
