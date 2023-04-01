#include <benchmark/benchmark.h>

#include <parsecpp/full.hpp>
#include <iostream>


using namespace prs;

struct ModifierPerfectPass {
    auto operator()(auto& parser, Stream& stream) const noexcept {
        using P = Parser<Unit>;
        if (!stream.eos()) {
            char c = stream.front();
            stream.moveUnsafe();
            auto result = parser();
            if (!result.isError() && !stream.eos()) {
                if (c != stream.front()) {
                    return P::makeError(stream.pos());
                } else {
                    stream.moveUnsafe();
                    return P::data({});
                }
            } else {
                return P::makeError(stream.pos());
            }
        } else {
            return P::makeError(stream.pos());
        }
    }
};


struct ModifierVirtualPass {
    auto operator()(ModifyCallerI<char>& parser, Stream& stream) const noexcept {
        using P = Parser<Unit>;
        if (!stream.eos()) {
            char c = stream.front();
            stream.moveUnsafe();
            auto result = parser();
            if (!result.isError() && !stream.eos()) {
                if (c != stream.front()) {
                    return P::makeError(stream.pos());
                } else {
                    stream.moveUnsafe();
                    return P::data({});
                }
            } else {
                return P::makeError(stream.pos());
            }
        } else {
            return P::makeError(stream.pos());
        }
    }
};

template <ParserType Parser>
static void BM_Modifier(benchmark::State& state, Parser const& parser, std::string_view challenge) {
    for (auto _ : state) {
        Stream s{challenge};

        auto ans = parser.apply(s);
        if (ans.isError()) {
            throw std::runtime_error("Cannot parse");
        }
    }
}

static inline std::string COMMON_CHALLENGE = details::repeatF<6>(std::string{"aba"}, [](auto s) {return s + s;});

static inline auto parserPerfect = (charFrom('b') * ModifierPerfectPass{}).repeat<1<<6>().endOfStream();
static inline auto parserVirtual = (charFrom('b') * ModifierVirtualPass{}).repeat<1<<6>().endOfStream();

static inline auto parserPerfectDrop = (charFrom('b') * ModifierPerfectPass{}).drop().repeat().endOfStream();
static inline auto parserVirtualDrop = (charFrom('b') * ModifierVirtualPass{}).drop().repeat().endOfStream();

BENCHMARK_CAPTURE(BM_Modifier, Perfect, parserPerfect, COMMON_CHALLENGE);
BENCHMARK_CAPTURE(BM_Modifier, Virtual, parserVirtual, COMMON_CHALLENGE);

BENCHMARK_CAPTURE(BM_Modifier, PerfectDrop, parserPerfectDrop, COMMON_CHALLENGE);
BENCHMARK_CAPTURE(BM_Modifier, VirtualDrop, parserVirtualDrop, COMMON_CHALLENGE);

#ifdef ENABLE_HARD_BENCHMARK

static inline std::string CHALLENGE8 = details::repeatF<8>(std::string{"aba"}, [](auto s) {return s + s;});

static inline auto parserPerfect8 = details::repeatF<8>(charFrom('b') * ModifierPerfectPass{}, [](auto t) {return t >> t;});
static inline auto parserVirtual8 = details::repeatF<8>(charFrom('b') * ModifierVirtualPass{}, [](auto t) {return t >> t;});

BENCHMARK_CAPTURE(BM_Modifier, PerfectLong8, parserPerfect8, CHALLENGE8);
BENCHMARK_CAPTURE(BM_Modifier, VirtualLong8, parserVirtual8, CHALLENGE8);

#endif
