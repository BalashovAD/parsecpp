#pragma once

#include <parsecpp/core/parser.h>

namespace prs {

template <typename T>
class ModifyCallerI {
public:
    virtual details::ResultType<T> operator()() = 0;
};

template <ParserType Parser>
class ModifyCaller : public ModifyCallerI<parser_result_t<Parser>> {
public:
    ModifyCaller(Parser const& p, Stream& s) noexcept
        : m_parser(p)
        , m_stream(s) {}

    typename Parser::Result operator()() final {
        return m_parser(m_stream);
    }
private:
    Parser const& m_parser;
    Stream& m_stream;
};


template <ParserType Parser, ContextType StoredCtx>
class ModifyCallerCtx : public ModifyCallerI<parser_result_t<Parser>> {
public:
    ModifyCallerCtx(Parser const& p, Stream& s, StoredCtx& ctx) noexcept
        : m_parser(p)
        , m_stream(s)
        , m_ctx(ctx) {

    }

    typename Parser::Result operator()() final {
        return m_parser(m_stream, m_ctx);
    }
private:
    Parser const& m_parser;
    Stream& m_stream;
    StoredCtx& m_ctx;
};

template <typename ModifierClass, typename Ctx>
class ModifyWithContext {
public:
    template <typename ...Args>
    explicit ModifyWithContext(Args&& ...args) noexcept(std::is_nothrow_constructible_v<ModifierClass, Args...>)
        : m_modifier(std::forward<Args>(args)...) {

    }

    auto operator()(auto& caller, Stream& stream, auto& ctx) const noexcept {
        return m_modifier(caller, stream, ctx);
    }
private:
    ModifierClass m_modifier;
};


template <ParserType ParserA, typename Modify>
    requires(ParserA::nocontext && std::is_invocable_v<Modify, ModifyCallerI<parser_result_t<ParserA>>&, Stream&>)
auto operator*(ParserA parserA, Modify modifier) noexcept {
    return make_parser([parser = std::move(parserA), mod = std::move(modifier)](Stream& stream) {
        ModifyCaller p{parser, stream};
        return mod(p, stream);
    });
}


template <ParserType ParserA, typename Modify>
    requires(!ParserA::nocontext)
auto operator*(ParserA parserA, Modify modifier) noexcept {
    using Ctx = parser_ctx_t<ParserA>;
    return make_parser<Ctx>(
            [parser = std::move(parserA), mod = std::move(modifier)](Stream& stream, auto& ctx) {
        ModifyCallerCtx p{parser, stream, ctx};
        if constexpr (std::is_invocable_v<Modify, ModifyCallerI<parser_result_t<ParserA>>&, Stream&>) {
            return mod(p, stream);
        } else {
            return mod(p, stream, ctx);
        }
    });
}


template <ParserType ParserA, typename Modify, typename Ctx>
auto operator*(ParserA parserA, ModifyWithContext<Modify, Ctx> modifier) noexcept {
    using UCtx = UnionCtx<parser_ctx_t<ParserA>, Ctx>;
    return make_parser<UCtx>(
            [parser = std::move(parserA), mod = std::move(modifier)](Stream& stream, auto& ctx) {
        if constexpr (ParserA::nocontext) {
            ModifyCaller p{parser, stream};
            return mod(p, stream, ctx);
        } else {
            ModifyCaller p{parser, stream, ctx};
            return mod(p, stream, ctx);
        }
    });
}

}