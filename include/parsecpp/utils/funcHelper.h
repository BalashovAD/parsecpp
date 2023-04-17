#pragma once

#include <functional>
#include <memory>
#include <string>


namespace prs::details {

struct Id {
    template<typename T>
    auto operator()(T&& t) const noexcept {
        return std::forward<T>(t);
    }
};

struct ToString {
    template <typename T>
        requires (std::same_as<decltype(std::to_string(std::declval<T>())), std::string>)
    std::string operator()(T const& t) const noexcept {
        return std::to_string(t);
    }
};


struct MakeTuple {
    template <typename ...Args>
    auto operator()(Args &&...args) const noexcept {
        return std::make_tuple(std::forward<Args>(args)...);
    }
};


template <typename T>
struct MakeClass {
    template <class ...Args>
    decltype(auto) operator()(Args &&...args) const noexcept(std::is_nothrow_constructible_v<T, Args...>) {
        return T{std::forward<Args>(args)...};
    }
};

template <typename T>
struct MakeShared {
    template <class ...Args>
    auto operator()(Args &&...args) const {
        return std::make_shared<T>(std::forward<Args>(args)...);
    }
};


template <typename Fn>
class Y {
public:
    explicit Y(Fn fn) noexcept
        : m_fn(fn)
    {}

    template <typename ...Args>
    decltype(auto) operator()(Args&&...args) const {
        return m_fn(*this, std::forward<Args>(args)...);
    }

private:
    Fn m_fn;
};


template <size_t pw, typename T, typename Fn>
constexpr decltype(auto) repeatF(T t, Fn op) noexcept(std::is_nothrow_invocable_v<Fn, T>) {
    if constexpr (pw == 0) {
        return t;
    } else {
        return repeatF<pw - 1>(op(t), op);
    }
}

}
