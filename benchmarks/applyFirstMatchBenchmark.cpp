#include <benchmark/benchmark.h>

#include <parsecpp/full.hpp>

#include "benchmarkHelper.hpp"

template <size_t testSize = 10000>
inline auto generateChallenge(size_t maxN) noexcept {
    std::array<unsigned, testSize> out{};

    for (size_t i = 0; i != testSize; ++i) {
        out[i] = ((i * i * 13 + 7 * i + 3) % maxN);
    }

    return out;
}

template <typename ApplyFirstMatchT>
void BM_ApplyFirstMatch(benchmark::State& state, ApplyFirstMatchT const& applyFirstMatch, size_t maxN) {
    auto tests = generateChallenge(maxN);
    for (auto _ : state) {
        for (auto const& t : tests) {
            benchmark::DoNotOptimize(applyFirstMatch.apply(t));
        }
    }
}


template <typename Map>
void BM_ApplyFirstMatchMap(benchmark::State& state, Map p, size_t size, size_t maxN) {
    auto tests = generateChallenge(maxN);
    for (unsigned i = 1; i != size; ++i) {
        p[i] = i;
    }
    const auto get = [&p](unsigned k) noexcept {
        if (auto it = p.find(k); it != p.end()) {
            return it->second;
        } else {
            return 0u;
        }
    };
    for (auto _ : state) {
        for (auto const& t : tests) {
            benchmark::DoNotOptimize(get(t));
        }
    }
}

void BM_ApplyFirstMatchSwitch3(benchmark::State& state, size_t maxN) {
    auto tests = generateChallenge(maxN);
    const auto get = [](unsigned k) noexcept {
        switch (k) {
            case 1:
                return 11;
            case 2:
                return 22;
            case 3:
                return 31;
            default:
                return 0;
        }
    };
    for (auto _ : state) {
        for (auto const& t : tests) {
            benchmark::DoNotOptimize(get(t));
        }
    }
}

void BM_ApplyFirstMatchSwitch9(benchmark::State& state, size_t maxN) {
    auto tests = generateChallenge(maxN);
    const auto get = [](unsigned k) noexcept {
        switch (k) {
            case 1: return 11;
            case 2: return 22;
            case 3: return 31;
            case 4: return 43;
            case 5: return 54;
            case 6: return 62;
            case 7: return 71;
            case 8: return 84;
            case 9: return 92;
            default:
                return 0;
        }
    };
    for (auto _ : state) {
        for (auto const& t : tests) {
            benchmark::DoNotOptimize(get(t));
        }
    }
}

constexpr auto a3 = prs::details::makeFirstMatch(0, std::make_tuple(1, 1), std::make_tuple(2, 1), std::make_tuple(3, 1));
constexpr auto a9 = prs::details::makeFirstMatch(0,
             std::make_tuple(1, 1), std::make_tuple(2, 2), std::make_tuple(3, 3),
            std::make_tuple(4, 4), std::make_tuple(5, 5), std::make_tuple(6, 6),
        std::make_tuple(7, 7), std::make_tuple(8, 8), std::make_tuple(9, 9));

constexpr auto a18 = prs::details::makeFirstMatch(0,
             std::make_tuple(1, 1), std::make_tuple(2, 2), std::make_tuple(3, 3),
            std::make_tuple(4, 4), std::make_tuple(5, 5), std::make_tuple(6, 6),
        std::make_tuple(7, 7), std::make_tuple(8, 8), std::make_tuple(9, 9),
        std::make_tuple(10, 10), std::make_tuple(11, 11), std::make_tuple(12, 12),
            std::make_tuple(13, 13), std::make_tuple(14, 14), std::make_tuple(15, 15),
        std::make_tuple(16, 16), std::make_tuple(17, 17), std::make_tuple(18, 18));

using Map = std::map<unsigned, unsigned>;
using UnorderedMap = std::unordered_map<unsigned, unsigned>;

#ifdef ENABLE_HARD_BENCHMARK

BENCHMARK_CAPTURE(BM_ApplyFirstMatch, Value3Items, a3, 4);
BENCHMARK_CAPTURE(BM_ApplyFirstMatchSwitch3, If3Items, 4);
BENCHMARK_CAPTURE(BM_ApplyFirstMatch, Value9Items, a9, 10);
BENCHMARK_CAPTURE(BM_ApplyFirstMatchSwitch9, If9Items, 10);
BENCHMARK_CAPTURE(BM_ApplyFirstMatchMap, Map9Items, Map{}, 9, 10);
BENCHMARK_CAPTURE(BM_ApplyFirstMatchMap, UMap9Items, UnorderedMap{}, 9, 10);
BENCHMARK_CAPTURE(BM_ApplyFirstMatch, Value9ItemsMiss, a9, 20);
BENCHMARK_CAPTURE(BM_ApplyFirstMatchSwitch9, If9ItemsMiss, 20);
BENCHMARK_CAPTURE(BM_ApplyFirstMatchMap, Map9ItemsMiss, Map{}, 9, 20);
BENCHMARK_CAPTURE(BM_ApplyFirstMatchMap, UMap9ItemsMiss, UnorderedMap{}, 9, 20);
BENCHMARK_CAPTURE(BM_ApplyFirstMatch, Value18Items, a18, 19);
BENCHMARK_CAPTURE(BM_ApplyFirstMatchMap, Map18Items, Map{}, 18, 19);
BENCHMARK_CAPTURE(BM_ApplyFirstMatchMap, UMap18Items, UnorderedMap{}, 18, 19);

#else

BENCHMARK_CAPTURE(BM_ApplyFirstMatch, Value9ItemsMiss, a9, 20);
BENCHMARK_CAPTURE(BM_ApplyFirstMatchSwitch9, If9ItemsMiss, 20);
BENCHMARK_CAPTURE(BM_ApplyFirstMatchMap, Map9ItemsMiss, Map{}, 9, 20);
BENCHMARK_CAPTURE(BM_ApplyFirstMatchMap, UMap9ItemsMiss, UnorderedMap{}, 9, 20);

#endif
