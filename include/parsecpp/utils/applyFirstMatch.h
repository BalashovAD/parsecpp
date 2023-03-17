#pragma once

#include <functional>
#include <tuple>

namespace prs::details {

namespace impl {

template<typename T>
struct strip_reference_wrapper {
    using type = T;
};

template <typename T>
struct strip_reference_wrapper<std::reference_wrapper<T>> {
    using type = T&;
};

}

template <typename T>
using strip_reference_wrapper_t = typename impl::strip_reference_wrapper<T>::type;

template <typename UnhandledAction, typename Equal, typename ...TupleArgs>
class ApplyFirstMatch {
public:
    using UnhandledType = UnhandledAction;
    static constexpr bool isRef = std::is_lvalue_reference_v<strip_reference_wrapper_t<UnhandledAction>>;

    static_assert(
            (isRef && (std::is_lvalue_reference_v<std::tuple_element_t<1, TupleArgs>> && ...))
            || (!isRef && (!std::is_lvalue_reference_v<std::tuple_element_t<1, TupleArgs>> && ...)));

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

        return foreach<0>(f, std::forward<Args>(args)...);
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

    template <unsigned shift, typename ...Args>
    constexpr decltype(auto) foreach(auto const& checkFn, Args &&...args) const {
        auto const& [key, fn] = get<shift>(m_tuple);
        if (checkFn(key)) {
            return invoke(fn, std::forward<Args>(args)...);
        } else {
            if constexpr (std::tuple_size_v<decltype(m_tuple)> > shift + 1) {
                return foreach<shift + 1>(checkFn, std::forward<Args>(args)...);
            } else {
                return invoke(m_unhandled, std::forward<Args>(args)...);
            }
        }
    }

    std::tuple<TupleArgs...> const m_tuple;
    strip_reference_wrapper_t<UnhandledAction> const m_unhandled;
    Equal const m_cmp;
};


template <typename UnhandledAction, typename ...TupleArgs>
static constexpr auto makeFirstMatch(UnhandledAction unhandled, TupleArgs &&...args) noexcept {
    return ApplyFirstMatch(std::move(unhandled), std::equal_to<>{}, std::forward<TupleArgs>(args)...);
}

//
//template <typename UnhandledActionT, typename ...TupleArgs>
//static constexpr auto makeFirstMatch(std::reference_wrapper<UnhandledActionT> unhandled, TupleArgs &&...args) noexcept {
//    return ApplyFirstMatch<UnhandledActionT&, decltype(std::equal_to<>{}), TupleArgs...>(unhandled.get(), std::equal_to<>{}, std::forward<TupleArgs>(args)...);
//}

template <typename UnhandledAction, typename Equal, typename ...TupleArgs>
static constexpr auto makeFirstMatchEq(UnhandledAction&& unhandled, Equal eq, TupleArgs &&...args) noexcept {
    return ApplyFirstMatch(std::forward<UnhandledAction>(unhandled), std::move(eq), std::forward<TupleArgs>(args)...);
}

}
