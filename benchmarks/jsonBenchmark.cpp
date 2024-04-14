#include <benchmark/benchmark.h>

#include <parsecpp/full.hpp>

#include "benchmarkHelper.hpp"
#include "./jsonImplLazy.cpp"

#include <variant>
#include <iostream>


using namespace prs;


template <typename Fn>
static void BM_jsonFile(benchmark::State& state, std::string filename, Fn parseAny) {
    auto parser = parseAny();
    std::string json = readFile(filename);
    for (auto _ : state) {
        Stream s(json);
        auto data = parser(s);
        if (data.isError()) {
            state.SkipWithError("Cannot parse json");
        }
        benchmark::DoNotOptimize(data.data());
    }

    state.SetBytesProcessed(json.size() * state.iterations());
}

#define BENCH_JSON(ns) \
\
BENCHMARK_CAPTURE(BM_jsonFile, 100k,  "100k.json", impl##ns::parseAny)->Name("BM_jsonFile_100k/"#ns);\
BENCHMARK_CAPTURE(BM_jsonFile, canada, "canada.json", impl##ns::parseAny)->Name("BM_jsonFile_canada/" #ns);\
BENCHMARK_CAPTURE(BM_jsonFile, binance, "binance.json", impl##ns::parseAny)->Name("BM_jsonFile_binance/" #ns);

#define BENCH_JSON_HARD(ns) \
\
BENCHMARK_CAPTURE(BM_jsonFile, 64kb, "64kb.json", impl##ns::parseAny)->Name("BM_jsonFile_64kb/" #ns);\
BENCHMARK_CAPTURE(BM_jsonFile, 64kb_min, "64kb-min.json", impl##ns::parseAny)->Name("BM_jsonFile_64kb_min/" #ns);\
\
BENCHMARK_CAPTURE(BM_jsonFile, 256kb, "256kb.json", impl##ns::parseAny)->Name("BM_jsonFile_256kb/" #ns);\
BENCHMARK_CAPTURE(BM_jsonFile, 256kb_min, "256kb-min.json", impl##ns::parseAny)->Name("BM_jsonFile_256kb_min/" #ns); \
\
BENCHMARK_CAPTURE(BM_jsonFile, 5mb, "5mb.json", impl##ns::parseAny)->Name("BM_jsonFile_5mb/" #ns);\
BENCHMARK_CAPTURE(BM_jsonFile, 5mb_min, "5mb-min.json", impl##ns::parseAny)->Name("BM_jsonFile_5mb_min/" #ns);


BENCH_JSON(Lazy);
BENCH_JSON(LazyCached);
BENCH_JSON(SelfLazy);

#ifdef ENABLE_HARD_BENCHMARK
BENCH_JSON_HARD(Lazy);
BENCH_JSON_HARD(LazyCached);
BENCH_JSON_HARD(SelfLazy);
#endif

#undef BENCH_JSON
