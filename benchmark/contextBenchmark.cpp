#include <benchmark/benchmark.h>

#include <parsecpp/all.hpp>

#include <variant>
#include <iostream>


using namespace prs;

// ctx >>
template <typename ParserA, typename ParserB>
auto ctxCombine(ParserA a, ParserB b) noexcept {
    using B = parser_result_t<ParserB>;
    return Parser<B, VoidContext>::make([a, b](Stream& stream, auto& ctx) {
        return a.apply(stream, ctx).flatMap([&b, &stream, &ctx](auto const& body) {
            return b.apply(stream, ctx);
        });
    });
}


template <ParserType Parser>
static void BM_Context(benchmark::State& state, Parser const& parser, std::string_view challenge) {
    for (auto _ : state) {
        Stream s{challenge};

        auto ans = parser.apply(s);
        if (ans.isError()) {
            throw std::runtime_error("Cannot parse");
        } else {

        }
    }
}

template <size_t pw, typename T, typename Fn>
auto repeatF(T t, Fn op) noexcept {
    if constexpr (pw == 0) {
        return t;
    } else {
        return repeatF<pw - 1>(op(t), op);
    }
}

static inline std::string_view COMMON_CHALLENGE = "abc abc  abc abc abc abc abcabc abc  abc abc abc abc abcabc abc  abc abc abc abc abcabc abc  abc abc abc abc abc";

static inline auto parserNoCtx = (charFrom('a') >> charFrom('b') >> charFrom('c') << spacesFast()).repeat<7 * 4>();
static inline auto parserCtx = (ctxCombine(ctxCombine(charFrom('a'), charFrom('b')), charFrom('c')) << spacesFast()).repeat<7 * 4>();

BENCHMARK_CAPTURE(BM_Context, Ctx, parserCtx, COMMON_CHALLENGE);
BENCHMARK_CAPTURE(BM_Context, Etalon, parserNoCtx, COMMON_CHALLENGE);

static inline auto parserNoCtxLong = repeatF<8>(charFrom('a'), [](auto p) {return p >> p;}).endOfStream();
static inline auto parserCtxLong = repeatF<8>(charFrom('a'), [](auto p) {return ctxCombine(p, p);}).endOfStream();

static inline std::string LONG_CHALLENGE = repeatF<8>(std::string{"a"}, [](auto s) {return s + s;});

BENCHMARK_CAPTURE(BM_Context, CtxLong8, parserCtxLong, LONG_CHALLENGE);
BENCHMARK_CAPTURE(BM_Context, EtalonLong8, parserNoCtxLong, LONG_CHALLENGE);

static inline auto parserCtxLong12 = repeatF<12>(charFrom('a'), [](auto p) {return ctxCombine(p, p);}).endOfStream();
static inline auto parserNoCtxLong12 = repeatF<12>(charFrom('a'), [](auto p) {return p >> p;}).endOfStream();

static inline std::string LONG_CHALLENGE12 = repeatF<12>(std::string{"a"}, [](auto s) {return s + s;});

BENCHMARK_CAPTURE(BM_Context, CtxLong12, parserCtxLong12, LONG_CHALLENGE12);
BENCHMARK_CAPTURE(BM_Context, EtalonLong12, parserNoCtxLong12, LONG_CHALLENGE12);
