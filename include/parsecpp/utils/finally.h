#pragma once

#include <concepts>
#include <utility>


namespace prs {

template <std::invocable Fn>
class Finally {
public:
    template <typename F>
        requires(std::is_constructible_v<Fn, F>)
    explicit Finally(F&& fn) noexcept(std::is_nothrow_constructible_v<Fn, F>)
        : m_fn(std::forward<F>(fn)) {

    }

    Finally(Finally const&) = delete;

    Finally(Finally&& rhs) noexcept(std::is_nothrow_move_assignable_v<Fn>)
        : m_own(std::exchange(rhs.m_own, false))
        , m_fn(std::move(rhs.m_fn)) {
    }

    Finally& operator=(Finally&&) = delete;

    void release() noexcept {
        m_own = false;
    }

    ~Finally() {
        if (m_own) {
            m_fn();
        }
    }
private:
    bool m_own = true;
    Fn m_fn;
};

template <typename Fn>
Finally(Fn) -> Finally<std::decay_t<Fn>>;

}