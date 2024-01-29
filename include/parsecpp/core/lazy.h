#pragma once

#include <parsecpp/core/parser.h>
#include <parsecpp/utils/funcHelper.h>

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


template <typename RetType, typename Fn>
auto selfLazy(Fn parserGen) noexcept {
    return Parser<RetType>::make([parserGen](Stream& stream) {
        details::YParser t{[&](auto const& self, Stream& s) -> Parser<RetType>::Result {
            return parserGen(Parser<RetType>::make([&](Stream& s) -> Parser<RetType>::Result {
                return self(s);
            })).apply(s);
        }};

        using YT = std::decay_t<decltype(t)>;
        static_assert(std::is_invocable_v<YT, Stream&>);
        static_assert(IsParserFn<YT, VoidContext>);
//        static_assert(IsParserFn<details::YParser<Fn, VoidContext>, VoidContext>);

        return t(stream);
    });
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


struct LazyBindingTag{};
template <typename T, ContextType ParserCtx, typename Tag = LazyBindingTag>
class LazyCtxBinding {
public:
    struct LazyContext;

    static constexpr bool nocontext = false;
    using Ctx = UnionCtx<ParserCtx, ContextWrapper<LazyContext const&>>;

    using P = Parser<T, Ctx>;
    using ParserResult = T;

    struct LazyContext {
        explicit LazyContext(void const*const p) : parser(p) {};
        void const*const parser;
    };

    explicit LazyCtxBinding() noexcept = default;

    template <ContextType Context>
    details::ResultType<ParserResult> operator()(Stream& stream, Context& ctx) const {
        auto const*const parser = reinterpret_cast<P const*const>(get<LazyContext>(ctx).parser);
        return parser->apply(stream, ctx);
    }
};

template <typename Fn, typename Tag>
auto lazyCtxBindingFn(Fn const& f [[maybe_unused]], Tag = {}) noexcept {
    using P = std::invoke_result_t<Fn>;
    using T = GetParserResult<P>;
    using Ctx = GetParserCtx<P>;
    using LZ = LazyCtxBinding<T, Ctx, Tag>;
    return Parser<T, typename LZ::Ctx>::make(LZ{});
}


template <typename Tag, typename Fn>
auto lazyCtxBindingFn(Fn const& f [[maybe_unused]]) noexcept {
    return lazyCtxBinding(f, Tag{});
}


template <typename T, typename Tag = LazyBindingTag>
auto lazyCtxBinding() noexcept {
    using LZ = LazyCtxBinding<T, VoidContext, Tag>;
    return Parser<T, typename LZ::Ctx>::make(LZ{});
}


template <typename T, typename Tag>
auto lazyCtxBinding(Tag) noexcept {
    return lazyCtxBinding<T, Tag>();
}

template <typename T, typename Ctx, typename Tag = LazyBindingTag>
auto lazyCtxBindingCtx() noexcept {
    using LZ = LazyCtxBinding<T, Ctx, Tag>;
    return Parser<T, typename LZ::Ctx>::make(LZ{});
}

template <typename T, typename Ctx, typename Tag>
auto lazyCtxBindingCtx(Tag = Tag{}) noexcept {
    return lazyCtxBindingCtx<T, Ctx, Tag>();
}

template <typename T, ContextType Ctx, typename Tag = LazyBindingTag>
requires (sizeCtx<Ctx> == 1)
auto makeBindingCtx(Parser<T, Ctx> const& parser) noexcept {
    using LazyCtx = typename LazyCtxBinding<T, VoidContext, Tag>::LazyContext;
    return LazyCtx{std::addressof(parser)};
}

}