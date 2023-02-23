#pragma once

#include <parsecpp/core/parser.h>

#include <concepts>

namespace prs {

template <std::invocable Fn>
auto lazy(Fn genParser) noexcept {
    using ParserResult = parser_result_t<std::invoke_result_t<Fn>>;
    return Parser<ParserResult>::make([genParser](Stream& stream) {
        return genParser().apply(stream);
    });
}


template <typename tag, typename Fn>
struct LazyCached {
    using ParserResult = parser_result_t<std::invoke_result_t<Fn>>;

    explicit LazyCached(Fn const& generator) noexcept {
        if (!insideWork) { // cannot use optional because generator() invoke ctor before optional::emplace
            insideWork = true;
            cachedParser.emplace(generator());
        }
    }

    auto operator()(Stream& stream) const {
        return (*cachedParser)(stream);
    }

    inline static bool insideWork = false;
    inline static std::optional<std::invoke_result_t<Fn>> cachedParser;
};


template <auto = details::SourceLocation::current().hash()>
struct AutoTag;

template <typename Tag = AutoTag<>, std::invocable Fn>
auto lazyCached(Fn const& genParser) noexcept(std::is_nothrow_invocable_v<Fn>) {
    return make_parser(LazyCached<Tag, Fn>{genParser});
}


template <typename T, typename tag = AutoTag<>>
struct LazyForget {
    using InvokerType = typename Parser<T>::Result (*)(void*, Stream&);

    template <typename ParserOut>
    InvokerType makeInvoker() noexcept {
        return [](void* pParser, Stream& s) {
            return static_cast<ParserOut*>(pParser)->apply(s);
        };
    }
public:
    template <typename Fn>
        requires(!std::is_same_v<Fn, LazyForget<T>>)
    explicit LazyForget(Fn const& fn) noexcept(std::is_nothrow_invocable_v<Fn>) {
        if (pInvoke == nullptr) {
            using ParserOut = std::invoke_result_t<Fn>;
            pInvoke = makeInvoker<ParserOut>();
            pParser = new ParserOut(fn());
        }
    }


    auto operator()(Stream& stream) const {
        return pInvoke(pParser, stream);
    }
private:
    static inline InvokerType pInvoke = nullptr;
    static inline void* pParser = nullptr;
};


template <typename T, typename Fn, typename Tag = AutoTag<>>
auto lazyForget(Fn const& f) noexcept {
    return Parser<T>::make(LazyForget<T, Tag>{f});
}

}