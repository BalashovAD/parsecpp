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
BENCHMARK_CAPTURE(BM_jsonFile, 100k, "100k.json", ns::parseAny);\
BENCHMARK_CAPTURE(BM_jsonFile, canada, "canada.json", ns::parseAny);\
BENCHMARK_CAPTURE(BM_jsonFile, binance, "binance.json", ns::parseAny);\

#define BENCH_JSON_HARD(ns) \
BENCHMARK_CAPTURE(BM_jsonFile, 64kb, "64kb.json", ns::parseAny);\
BENCHMARK_CAPTURE(BM_jsonFile, 64kb_min, "64kb-min.json", ns::parseAny);\
\
BENCHMARK_CAPTURE(BM_jsonFile, 256kb, "256kb.json", ns::parseAny);\
BENCHMARK_CAPTURE(BM_jsonFile, 256kb_min, "256kb-min.json", ns::parseAny);\
\
BENCHMARK_CAPTURE(BM_jsonFile, 5mb, "5mb.json", ns::parseAny);\
BENCHMARK_CAPTURE(BM_jsonFile, 5mb_min, "5mb-min.json", ns::parseAny);\
\

BENCH_JSON(implLazy);

#ifdef ENABLE_HARD_BENCHMARK
BENCH_JSON_HARD(implLazy);
#endif

#undef BENCH_JSON
