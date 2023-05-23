#include <benchmark/benchmark.h>

#include <parsecpp/all.hpp>

#include <variant>
#include <iostream>

using namespace prs;

bool checkBraces(std::tuple<char, char> const& cc) noexcept {
    return details::cmpAnyOf(cc
             , std::make_tuple('(', ')')
             , std::make_tuple('{', '}')
             , std::make_tuple('[', ']'));
}

Parser<Unit> bracesLazy() noexcept {
    return (concat(charFrom('(', '{', '['), lazy(bracesLazy) >> charFrom(')', '}', ']'))
        .cond(checkBraces).repeat<5>() >> success()).toCommonType();
}


Parser<Unit> bracesCache() noexcept {
    return (concat(charFrom('(', '{', '['), lazyCached(bracesCache, AutoTagV) >> charFrom(')', '}', ']'))
        .cond(checkBraces).repeat<5>() >> success()).toCommonType();
}


Parser<Drop> bracesCacheDrop() noexcept {
    return (concat(charFrom('(', '{', '['), lazyCached(bracesCacheDrop, AutoTagV) >> charFrom(')', '}', ']'))
        .cond(checkBraces).drop().repeat<>()).toCommonType();
}

using ForgetTag = AutoTagT;
auto bracesForget() noexcept -> decltype((concat(charFrom('(', '{', '['), std::declval<Parser<Unit, VoidContext, LazyForget<Unit, ForgetTag>>>() >> charFrom(')', '}', ']'))
        .cond(checkBraces).repeat<5>() >> success())) {

    return (concat(charFrom('(', '{', '['), lazyForget<Unit, ForgetTag>(bracesForget) >> charFrom(')', '}', ']'))
        .cond(checkBraces).repeat<5>() >> success());
}

template <ParserType P>
void BM_bracesSuccess(benchmark::State& state, P parser) {
    std::string_view braces = "{}([{()()}[]]){{}[]()}";
    for (auto _ : state) {
        Stream s{braces};
        auto result = parser(s);
        if (result.isError()) {
            throw std::runtime_error("Cannot parse braces");
        }
    }
}


template <ParserType P>
void BM_bracesFailure(benchmark::State& state, P parser) {
    std::string_view braces = "{}([{()()}[]])]{{}[]()}";
    for (auto _ : state) {
        Stream s{braces};
        auto result = parser(s);
        if (!result.isError()) {
            throw std::runtime_error("Error parse braces");
        }
    }
}

BENCHMARK_CAPTURE(BM_bracesSuccess, bracesLazy, bracesLazy().endOfStream());
BENCHMARK_CAPTURE(BM_bracesSuccess, bracesCached, bracesCache().endOfStream());
BENCHMARK_CAPTURE(BM_bracesSuccess, bracesCachedDrop, bracesCacheDrop().endOfStream());
BENCHMARK_CAPTURE(BM_bracesSuccess, bracesForget, bracesForget().endOfStream());

BENCHMARK_CAPTURE(BM_bracesFailure, bracesLazyF, bracesLazy().endOfStream());
BENCHMARK_CAPTURE(BM_bracesFailure, bracesCachedF, bracesCache().endOfStream());
BENCHMARK_CAPTURE(BM_bracesFailure, bracesCachedDropF, bracesCacheDrop().endOfStream());
BENCHMARK_CAPTURE(BM_bracesFailure, bracesForgetF, bracesForget().endOfStream());