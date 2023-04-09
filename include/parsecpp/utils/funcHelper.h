#pragma once

#include <parsecpp/core/stream.h>

#include <functional>
#include <memory>

#include <string>


namespace prs {


struct Unit {
    Unit() noexcept = default;

    bool operator==(Unit const&) const noexcept {
        return true;
    }
};

struct Drop {
    Drop() noexcept = default;

    bool operator==(Drop const&) const noexcept {
        return true;
    }
};


template <typename Parser>
constexpr bool is_parser_v = std::is_invocable_v<Parser, Stream&>;

template <typename T>
concept ParserType = is_parser_v<T>;


template <ParserType Parser>
using parser_result_t = typename std::decay_t<Parser>::Type;


template <ParserType Parser>
using parser_ctx_t = typename std::decay_t<Parser>::Ctx;

static constexpr size_t MAX_ITERATION = 1000000;

namespace details {

struct Id {
    template<typename T>
    T operator()(T t) const noexcept {
        return t;
    }


    template<typename T>
    T operator()(T const& t) const noexcept {
        return t;
    }
};

struct ToString {
    template<typename T>
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
    auto operator()(Args &&...args) const noexcept {
        return T{args...};
    }
};

template <typename T>
struct MakeShared {
    template <class ...Args>
    auto operator()(Args &&...args) const noexcept {
        return std::make_shared<T>(args...);
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
constexpr decltype(auto) repeatF(T t, Fn op) noexcept {
    if constexpr (pw == 0) {
        return t;
    } else {
        return repeatF<pw - 1>(op(t), op);
    }
}

}
}
