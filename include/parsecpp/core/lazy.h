#pragma once

#include <parsecpp/core/parser.h>

#include <source_location>

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
    using ParserResult = GetParserResult<P>;
    static constexpr bool nocontext = IsVoidCtx<GetParserCtx<P>>;

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


template <auto tHash>
struct AutoTag {
    static constexpr auto value = tHash;
};

#define AutoTagT AutoTag<details::SourceLocation::current().hash()>
#define AutoTagV AutoTagT{}


template <typename Tag, std::invocable Fn>
auto lazyCached(Fn const& genParser) noexcept(std::is_nothrow_invocable_v<Fn>) {
    return make_parser(LazyCached<Tag, Fn>{genParser});
}

template <typename Tag, std::invocable Fn>
auto lazyCached(Fn const& genParser, Tag tag) noexcept(std::is_nothrow_invocable_v<Fn>) {
    return make_parser(LazyCached<Tag, Fn>{genParser});
}


template <typename T, typename tag>
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
        requires(!std::is_same_v<Fn, LazyForget<T, tag>>)
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


template <typename T, typename Ctx, typename tag>
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
        requires(!std::is_same_v<Fn, LazyForgetCtx<T, Ctx, tag>>)
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


template <typename T, typename Tag, typename Fn>
auto lazyForget(Fn const& f, Tag = {}) noexcept {
    return Parser<T>::make(LazyForget<T, Tag>{f});
}

template <typename T, typename Ctx, typename Fn, typename Tag>
auto lazyForgetCtx(Fn const& f, Tag = {}) noexcept {
    return Parser<T>::make(LazyForgetCtx<T, Ctx, Tag>{f});
}

}