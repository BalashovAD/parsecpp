#pragma once

#include <parsecpp/core/parser.h>
#include <parsecpp/common/base.h>

namespace prs {


template <typename Fn>
auto make_parser(Fn f) noexcept {
    return Parser<typename std::invoke_result_t<Fn, Stream&>::Body>::make(f);
}

namespace details {

template <typename Fn, typename TupleParser, typename ...Values>
auto liftRec(Fn fn, TupleParser parsers, Values ...values) noexcept {
    constexpr size_t Ind = sizeof...(values);
    if constexpr (std::tuple_size_v<TupleParser> == Ind) {
        return pure(std::invoke(fn, values...));
    } else {
        return std::get<Ind>(parsers).flatMap([&values..., fn, parsers](auto const& a) {
            return liftRec<Fn, TupleParser, Values..., decltype(a)>(fn, parsers, values..., a);
        });
    }
}

}

template <typename Fn, typename ...Args>
auto liftM(Fn fn, Args &&...args) noexcept {
    return details::liftRec<Fn>(fn, std::make_tuple(std::forward<Args>(args)...));
}

template <typename ...Args>
auto concat(Args &&...args) noexcept {
    return liftM(details::MakeTuple{}, std::forward<Args>(args)...);
}

template <typename Fn>
auto satisfy(Fn test) noexcept {
    return Parser<char>::make([test](Stream& stream) {
        if (auto c = stream.checkFirst(test); c != 0) {
            return Parser<char>::data(c);
        } else {
            return Parser<char>::error("satisfy", stream.pos());
        }
    });
}

}