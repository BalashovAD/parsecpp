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
        return std::invoke(m_fn, *this, std::forward<Args>(args)...);
    }

private:
    Fn m_fn;
};


// This implementation of Y combinator with implicit return type is needed because
// we can meet recursive initialization in case then Fn is lambda and capture a concept with call check, like a Parser
// Error: function 'operator()<>' with deduced return type cannot be used before it is defined
// while substituting into a lambda expression here
// while substituting template arguments into constraint expression here `requires (IsParserFn<Fn, Ctx>)`
template <typename T, typename Fn>
class YY {
public:
    explicit YY(Fn fn) noexcept
        : m_fn(fn)
    {}

    template <typename ...Args>
    T operator()(Args&&...args) const {
        return std::invoke(m_fn, *this, std::forward<Args>(args)...);
    }

private:
    Fn m_fn;
};

/**
 * The next  code sometimes broken for deducting type, so YParser is
template <typename ...Args>
decltype(auto) operator()(Args&&...args) const {
    return std::invoke(m_fn, *this, std::forward<Args>(args)...);
}
 */
template <typename Fn, typename Ctx>
class YParser;

template <typename Fn>
class YParser<Fn, VoidContext> {
public:
    explicit YParser(Fn fn) noexcept
        : m_fn(fn)
    {}

    std::invoke_result_t<Fn, YParser<Fn, VoidContext> const&, Stream&> operator()(Stream& s) const {
        return std::invoke(m_fn, *this, s);
    }
private:
    Fn m_fn;
};

template <typename Fn, typename Ctx>
class YParser {
public:
    explicit YParser(Fn fn) noexcept
        : m_fn(fn)
    {}

//    decltype(auto) operator()(Stream& s, Ctx& ctx) const {
//        return std::invoke(m_fn, *this, s, ctx);
//    }

    decltype(auto) operator()(Stream& s) const {
        return std::invoke(m_fn, *this, s);
    }
private:
    Fn m_fn;
};

template <typename Fn>
YParser(Fn f) -> YParser<Fn, VoidContext>;


template <size_t pw, typename T, typename Fn>
constexpr decltype(auto) repeatF(T t, Fn op) noexcept(std::is_nothrow_invocable_v<Fn, T>) {
    if constexpr (pw == 0) {
        return t;
    } else {
        return repeatF<pw - 1>(op(t), op);
    }
}

}
