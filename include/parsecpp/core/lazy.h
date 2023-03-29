#pragma once

#include <parsecpp/core/parser.h>

#include <concepts>

namespace prs {

template <std::invocable Fn>
auto lazy(Fn genParser) noexcept {
    using P = std::invoke_result_t<Fn>;
    if constexpr (P::nocontext) {
        return P::make([genParser](Stream& stream) {
            return genParser().apply(stream);
        });
    } else {
        return P::make([genParser](Stream& stream, auto& ctx) {
            return genParser().apply(stream, ctx);
        });
    }
}


template <typename tag, typename Fn>
class LazyCached {
public:
    using P = std::invoke_result_t<Fn>;
    using ParserResult = parser_result_t<P>;
    static constexpr bool nocontext = IsVoidCtx<parser_ctx_t<P>>;

    explicit LazyCached(Fn const& generator) noexcept {
        if (!insideWork) { // cannot use optional.has_value() because generator() invoke ctor before optional::emplace
            insideWork = true;
            cachedParser.emplace(generator());
        }
    }

    auto operator()(Stream& stream) const {
        return (*cachedParser)(stream);
    }

    auto operator()(Stream& stream, auto& ctx) const {
        return (*cachedParser)(stream, ctx);
    }
private:
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
class LazyForget {
    using InvokerType = typename Parser<T>::Result (*)(void*, Stream&);

    template <typename ParserOut>
    static InvokerType makeInvoker() noexcept {
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


template <typename T, typename Ctx, typename tag = AutoTag<>>
        requires (!IsVoidCtx<Ctx>)
class LazyForgetCtx {
    using InvokerType = typename Parser<T, Ctx>::Result (*)(void*, Stream&, Ctx&);

    template <typename ParserOut>
    static InvokerType makeInvoker() noexcept {
        return [](void* pParser, Stream& s, auto& ctx) {
            return static_cast<ParserOut*>(pParser)->apply(s, ctx);
        };
    }
public:
    template <typename Fn>
        requires(!std::is_same_v<Fn, LazyForget<T>>)
    explicit LazyForgetCtx(Fn const& fn) noexcept(std::is_nothrow_invocable_v<Fn>) {
        if (pInvoke == nullptr) {
            using ParserOut = std::invoke_result_t<Fn>;
            pInvoke = makeInvoker<ParserOut>();
            pParser = new ParserOut(fn());
        }
    }


    auto operator()(Stream& stream, Ctx& ctx) const {
        return pInvoke(pParser, stream, ctx);
    }
private:
    static inline InvokerType pInvoke = nullptr;
    static inline void* pParser = nullptr;
};


template <typename T, typename Fn, typename Tag = AutoTag<>>
auto lazyForget(Fn const& f) noexcept {
    return Parser<T>::make(LazyForget<T, Tag>{f});
}

template <typename T, typename Ctx, typename Fn, typename Tag = AutoTag<>>
auto lazyForgetCtx(Fn const& f) noexcept {
    return Parser<T>::make(LazyForgetCtx<T, Ctx, Tag>{f});
}

}