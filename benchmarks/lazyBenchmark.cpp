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

static constexpr size_t REPEAT_PRE_ALLOC = 30;

Parser<Unit> bracesLazy() noexcept {
    return (concat(charFrom('(', '{', '['), lazy(bracesLazy) >> charFrom(')', '}', ']'))
        .cond(checkBraces).repeat<REPEAT_PRE_ALLOC>() >> success()).toCommonType();
}


Parser<Unit> bracesCache() noexcept {
    return (concat(charFrom('(', '{', '['), lazyCached(bracesCache, AutoTagV) >> charFrom(')', '}', ']'))
        .cond(checkBraces).repeat<REPEAT_PRE_ALLOC>() >> success()).toCommonType();
}


Parser<Unit> bracesCacheConstexpr() noexcept {
    return (concat(charFrom<'(', '{', '['>(), lazyCached(bracesCacheConstexpr, AutoTagV) >> charFrom<')', '}', ']'>())
        .cond(checkBraces).repeat<REPEAT_PRE_ALLOC>() >> success()).toCommonType();
}


Parser<Drop> bracesCacheDrop() noexcept {
    return (concat(charFrom('(', '{', '['), lazyCached(bracesCacheDrop, AutoTagV) >> charFrom(')', '}', ']'))
        .cond(checkBraces).drop().repeat<>()).toCommonType();
}

Parser<Drop> bracesCacheDropConstexpr() noexcept {
    return (concat(charFrom<'(', '{', '['>(), lazyCached(bracesCacheDropConstexpr, AutoTagV) >> charFrom<')', '}', ']'>())
        .cond(checkBraces).drop().repeat<>()).toCommonType();
}

using ForgetTag = AutoTagT;
auto bracesForget() noexcept -> decltype((concat(charFrom('(', '{', '['), std::declval<Parser<Unit, VoidContext, LazyForget<Unit, ForgetTag>>>() >> charFrom(')', '}', ']'))
        .cond(checkBraces).repeat<REPEAT_PRE_ALLOC>() >> success())) {

    return (concat(charFrom('(', '{', '['), lazyForget<Unit, ForgetTag>(bracesForget) >> charFrom(')', '}', ']'))
        .cond(checkBraces).repeat<REPEAT_PRE_ALLOC>() >> success());
}

auto bracesCtx() noexcept {
    return (concat(charFrom('(', '{', '['), lazyCtxBinding<Unit>() >> charFrom(')', '}', ']'))
        .cond(checkBraces).repeat<REPEAT_PRE_ALLOC>() >> success()).toCommonType();
}

auto bracesCtxDrop() noexcept {
    return (concat(charFrom('(', '{', '['), lazyCtxBinding<Unit>() >> charFrom(')', '}', ']'))
        .cond(checkBraces).drop().repeat<>() >> success()).toCommonType();
}

auto bracesCtxDropConstexpr() noexcept {
    return (concat(charFrom<'(', '{', '['>(), lazyCtxBinding<Unit>() >> charFrom<')', '}', ']'>())
        .cond(checkBraces).drop().repeat<>() >> success()).toCommonType();
}

auto bracesCtxConstexpr() noexcept {
    return (concat(charFrom<'(', '{', '['>(), lazyCtxBinding<Unit>() >> charFrom<')', '}', ']'>())
        .cond(checkBraces).repeat<REPEAT_PRE_ALLOC>() >> success()).toCommonType();
}

static constexpr std::string_view braces = "{}(()()[{({}([{()()}[]]){{}[]()})()}[{}{{}[]()}([{()()}[]]){{}[{{}[]()}]()}]]){{}[{}([{()()}[]]){{}[]()}](){}([{()()}[{{}[]()}]]){{}[]()}}";
static constexpr std::string_view bracesFailed = "{}(()()[{({}([{()()}[]]){{}[]()})()}[{}{{}[]()}([{()()}[]]){{}[{{}[]()}]()}]]){{}[{}([{()()}[]]){{}[]()}](){}([{()()}[{{}[]()}]])){{}[]()}}";

template <ParserType P>
void BM_bracesSuccess(benchmark::State& state, P parser) {
    for (auto _ : state) {
        Stream s{braces};
        auto result = parser(s);
        if (result.isError()) {
            state.SkipWithError("Cannot parse braces");
        }
    }

    state.SetBytesProcessed(braces.size() * state.iterations());
}


template <ParserType P>
void BM_bracesFailure(benchmark::State& state, P parser) {
    for (auto _ : state) {
        Stream s{bracesFailed};
        auto result = parser(s);
        if (!result.isError()) {
            state.SkipWithError("Error parse braces");
        }
    }

    state.SetBytesProcessed(130 * state.iterations()); // First error in braces order
}


template <ParserType P>
void BM_bracesSuccessCtx(benchmark::State& state, P baseParser) {
    auto lazyBindingStorage = makeBindingCtx(baseParser);
    auto parser = baseParser.endOfStream();
    auto ctx = parser.makeCtx(lazyBindingStorage);
    for (auto _ : state) {
        Stream s{braces};
        auto result = parser(s, ctx);
        if (result.isError()) {
            state.SkipWithError("Cannot parse braces");
        }
    }

    state.SetBytesProcessed(braces.size() * state.iterations());
}


template <ParserType P>
void BM_bracesFailureCtx(benchmark::State& state, P baseParser) {
    auto lazyBindingStorage = makeBindingCtx(baseParser);
    auto parser = baseParser.endOfStream();
    auto ctx = parser.makeCtx(lazyBindingStorage);
    for (auto _ : state) {
        Stream s{bracesFailed};
        auto result = parser(s, ctx);
        if (!result.isError()) {
            state.SkipWithError("Error parse braces");
        }
    }

    state.SetBytesProcessed(130 * state.iterations()); // First error in braces order
}

BENCHMARK_CAPTURE(BM_bracesSuccess, bracesLazy, bracesLazy().endOfStream());
BENCHMARK_CAPTURE(BM_bracesSuccess, bracesCached, bracesCache().endOfStream());
#ifdef ENABLE_HARD_BENCHMARK
BENCHMARK_CAPTURE(BM_bracesSuccess, bracesCachedDrop, bracesCacheDrop().endOfStream());
BENCHMARK_CAPTURE(BM_bracesSuccess, bracesCacheDropConstexpr, bracesCacheDropConstexpr().endOfStream());
BENCHMARK_CAPTURE(BM_bracesSuccess, bracesCacheConstexpr, bracesCacheConstexpr().endOfStream());
#endif
BENCHMARK_CAPTURE(BM_bracesSuccess, bracesForget, bracesForget().endOfStream());
BENCHMARK_CAPTURE(BM_bracesSuccessCtx, bracesCtx, bracesCtx());
#ifdef ENABLE_HARD_BENCHMARK
BENCHMARK_CAPTURE(BM_bracesSuccessCtx, bracesCtxDrop, bracesCtxDrop());
BENCHMARK_CAPTURE(BM_bracesSuccessCtx, bracesCtxDropConstexpr, bracesCtxDropConstexpr());
BENCHMARK_CAPTURE(BM_bracesSuccessCtx, bracesCtxConstexpr, bracesCtxConstexpr());
#endif

BENCHMARK_CAPTURE(BM_bracesFailure, bracesLazy, bracesLazy().endOfStream());
BENCHMARK_CAPTURE(BM_bracesFailure, bracesCached, bracesCache().endOfStream());
#ifdef ENABLE_HARD_BENCHMARK
BENCHMARK_CAPTURE(BM_bracesFailure, bracesCachedDrop, bracesCacheDrop().endOfStream());
BENCHMARK_CAPTURE(BM_bracesFailure, bracesCacheDropConstexpr, bracesCacheDropConstexpr().endOfStream());
BENCHMARK_CAPTURE(BM_bracesFailure, bracesCacheConstexpr, bracesCacheConstexpr().endOfStream());
#endif
BENCHMARK_CAPTURE(BM_bracesFailure, bracesForget, bracesForget().endOfStream());
BENCHMARK_CAPTURE(BM_bracesFailureCtx, bracesCtx, bracesCtx());
#ifdef ENABLE_HARD_BENCHMARK
BENCHMARK_CAPTURE(BM_bracesFailureCtx, bracesCtxDrop, bracesCtxDrop());
BENCHMARK_CAPTURE(BM_bracesFailureCtx, bracesCtxDropConstexpr, bracesCtxDropConstexpr());
BENCHMARK_CAPTURE(BM_bracesFailureCtx, bracesCtxConstexpr, bracesCtxConstexpr());
#endif
