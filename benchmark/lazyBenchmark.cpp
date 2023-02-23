#include <benchmark/benchmark.h>

#include <parsecpp/all.h>

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
    return (concat(charIn('(', '{', '['), lazy(bracesLazy) >> charIn(')', '}', ']'))
        .cond(checkBraces).repeat<5>() >> success()).toCommonType();
}


Parser<Unit> bracesCache() noexcept {
    return (concat(charIn('(', '{', '['), lazyCached(bracesCache) >> charIn(')', '}', ']'))
        .cond(checkBraces).repeat<5>() >> success()).toCommonType();
}

auto bracesForget() noexcept -> decltype((concat(charIn('(', '{', '['), std::declval<Parser<Unit, LazyForget<Unit>>>() >> charIn(')', '}', ']'))
        .cond(checkBraces).repeat<5>() >> success())) {

    return (concat(charIn('(', '{', '['), lazyForget<Unit>(bracesForget) >> charIn(')', '}', ']'))
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
BENCHMARK_CAPTURE(BM_bracesSuccess, bracesForget, bracesForget().endOfStream());

BENCHMARK_CAPTURE(BM_bracesFailure, bracesLazyF, bracesLazy().endOfStream());
BENCHMARK_CAPTURE(BM_bracesFailure, bracesCachedF, bracesCache().endOfStream());
BENCHMARK_CAPTURE(BM_bracesFailure, bracesForgetF, bracesForget().endOfStream());