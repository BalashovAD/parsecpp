#pragma once

#include <parsecpp/core/parser.h>
#include <parsecpp/common/base.h>

namespace prs {


template <typename Ctx = VoidContext, typename Fn>
auto make_parser(Fn &&f) noexcept {
    using T = typename std::conditional_t<std::is_invocable_v<Fn, Stream&>
            , std::invoke_result<Fn, Stream&>
            , std::invoke_result<Fn, Stream&, Ctx&>>::type::Body;

    return Parser<T, Ctx>::make(std::forward<Fn>(f));
}

namespace details {

template <typename Fn, typename TupleParser, typename ...Values>
auto liftRec(Fn const& fn, Stream& stream, TupleParser const& parsers, Values &&...values) noexcept {
    constexpr size_t Ind = sizeof...(values);
    if constexpr (std::tuple_size_v<TupleParser> == Ind) {
        using ReturnType = std::invoke_result_t<Fn, Values...>;
        return Parser<ReturnType>::data(std::invoke(fn, values...));
    } else {
        return std::get<Ind>(parsers).apply(stream).flatMap([&](auto &&a) {
            return liftRec(
                    fn, stream, parsers, std::forward<Values>(values)..., a);
        });
    }
}


template <typename Fn, typename Ctx, typename TupleParser, typename ...Values>
auto liftRecCtx(Fn const& fn, Stream& stream, Ctx& ctx, TupleParser const& parsers, Values &&...values) noexcept {
    constexpr size_t Ind = sizeof...(values);
    if constexpr (std::tuple_size_v<TupleParser> == Ind) {
        using ReturnType = std::invoke_result_t<Fn, Values...>;
        return Parser<ReturnType>::data(std::invoke(fn, values...));
    } else {
        return std::get<Ind>(parsers).apply(stream, ctx).flatMap([&](auto &&a) {
            return liftRecCtx(
                    fn, stream, ctx, parsers, std::forward<Values>(values)..., a);
        });
    }
}

}

template <typename Fn, ParserType ...Args>
    requires(IsVoidCtx<GetParserCtx<Args>> && ...)
auto liftM(Fn fn, Args &&...args) noexcept {
    return make_parser([fn, parsers = std::make_tuple(args...)](Stream& s) {
        return details::liftRec(fn, s, parsers);
    });
}


template <typename Fn, ParserType ...Args>
    requires(!IsVoidCtx<GetParserCtx<Args>> || ...)
auto liftM(Fn fn, Args &&...args) noexcept {
    using Ctx = UnionCtx<GetParserCtx<Args>...>;
    return make_parser<Ctx>([fn, parsers = std::make_tuple(args...)](Stream& s, auto& ctx) {
        return details::liftRecCtx(fn, s, ctx, parsers);
    });
}

template <ParserType ...Args>
auto concat(Args &&...args) noexcept {
    return liftM(details::MakeTuple{}, std::forward<Args>(args)...);
}

template <typename Fn>
constexpr auto satisfy(Fn&& tTest) noexcept {
    return Parser<char>::make([test = std::forward<Fn>(tTest)](Stream& stream) {
        if (auto c = stream.checkFirst(test); c != 0) {
            return Parser<char>::data(c);
        } else {
            return Parser<char>::PRS_MAKE_ERROR("satisfy", stream.pos());
        }
    });
}

}