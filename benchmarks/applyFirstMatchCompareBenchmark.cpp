#include <benchmark/benchmark.h>

#include <parsecpp/full.hpp>

#include "benchmarkHelper.hpp"


constexpr auto SAMPLE_SIZE = 23;
static inline std::array<std::pair<std::string, unsigned>, SAMPLE_SIZE> samples = {
        std::make_pair("positions", 100),
        std::make_pair("balance_and_position", 40),
        std::make_pair("liquidation-warning", 10),
        std::make_pair("account-greeks", 5),
        std::make_pair("orders", 2),
        std::make_pair("orders-algo", 1),
        std::make_pair("algo-advance", 1),
        std::make_pair("filler1", 5),
        std::make_pair("filler2", 5),
        std::make_pair("filler3", 5),
        std::make_pair("filler4", 5),
        std::make_pair("filler5", 5),
        std::make_pair("filler6", 5),
        std::make_pair("filler7", 5),
        std::make_pair("filler8", 5),
        std::make_pair("filler9", 5),
        std::make_pair("filler10", 1),
        std::make_pair("filler11", 1),
        std::make_pair("filler12", 1),
        std::make_pair("filler13", 1),
        std::make_pair("filler14", 1),
        std::make_pair("filler15", 1),
        std::make_pair("filler16", 1)
};


template <size_t testCases = 10000>
inline auto generateChallengeSamplesWithoutWeights(size_t sampleSize) noexcept {
    assert(SAMPLE_SIZE >= sampleSize);
    std::array<std::string_view, testCases> out{};

    for (size_t i = 0; i != testCases; ++i) {
        out[i] = (samples[getRandomNumber(0, sampleSize - 1)].first);
    }

    return out;
}

template <size_t testCases = 10000>
inline auto generateChallengeSamplesWithWeights(size_t sampleSize) noexcept {
    assert(SAMPLE_SIZE >= sampleSize);
    std::array<std::string_view, testCases> out{};

    auto sum = std::accumulate(samples.begin(), samples.begin() + sampleSize, 0u, [](auto init, auto const& s) {
        return init + s.second;
    });

    for (size_t i = 0; i != testCases; ++i) {
        auto k = getRandomNumber(0, sum);
        auto currentSum = 0u;
        auto j = 0;
        while (currentSum <= k) {
            currentSum += samples[j].second;
            ++j;
        }
        out[i] = (samples[j].first);
    }

    return out;
}


template <typename ApplyFirstMatchT, typename Tests>
void BM_ApplyFirstMatchStrings(benchmark::State& state, ApplyFirstMatchT const& applyFirstMatch, Tests const& tests, size_t sampleCount) {
    if (sampleCount != applyFirstMatch.size()) {
        state.SkipWithError("Not equal sample count");
    }

    for (auto _ : state) {
        for (auto const& t : tests) {
            benchmark::DoNotOptimize(applyFirstMatch.apply(t));
        }
    }
    state.SetItemsProcessed(tests.size() * state.iterations());
}


template <typename Map, typename Tests>
void BM_ApplyFirstMatchMapStrings(benchmark::State& state, Map const& p, Tests const& tests, size_t sampleCount) {
    if (sampleCount != p.size()) {
        state.SkipWithError("Not equal sample count");
    }

    const auto get = [&p](auto k) noexcept -> int {
        if (auto it = p.find(k); it != p.end()) {
            return it->second;
        } else {
            return 5;
        }
    };
    for (auto _ : state) {
        for (auto const& t : tests) {
            benchmark::DoNotOptimize(get(t));
        }
    }
    state.SetItemsProcessed(tests.size() * state.iterations());
}

using namespace std::string_view_literals;

static inline constexpr auto apply4 = prs::details::makeFirstMatch(5,
               std::make_pair("positions"sv, 1),
               std::make_pair("balance_and_position"sv, 67),
               std::make_pair("liquidation-warning"sv, 11),
               std::make_pair("account-greeks"sv, 5));

static inline constexpr auto apply7 = prs::details::makeFirstMatch(5,
               std::make_pair("positions"sv, 1),
               std::make_pair("balance_and_position"sv, 67),
               std::make_pair("liquidation-warning"sv, 11),
               std::make_pair("account-greeks"sv, 5),
               std::make_pair("orders"sv, 2),
               std::make_pair("orders-algo"sv, 1),
               std::make_pair("algo-advance"sv, 121)
);

static inline constexpr auto apply12 = prs::details::makeFirstMatch(5,
               std::make_pair("positions"sv, 1),
               std::make_pair("balance_and_position"sv, 67),
               std::make_pair("liquidation-warning"sv, 11),
               std::make_pair("filler1"sv, 14),
               std::make_pair("filler2"sv, 115),
               std::make_pair("filler3"sv, 16),
               std::make_pair("filler4"sv, 17),
               std::make_pair("filler5"sv, 148),
               std::make_pair("account-greeks"sv, 5),
               std::make_pair("orders"sv, 2),
               std::make_pair("orders-algo"sv, 1),
               std::make_pair("algo-advance"sv, 121)
);

static inline constexpr auto apply18 = prs::details::makeFirstMatch(5,
               std::make_pair("positions"sv, 1),
               std::make_pair("balance_and_position"sv, 67),
               std::make_pair("liquidation-warning"sv, 11),
               std::make_pair("filler1"sv, 14),
               std::make_pair("filler2"sv, 115),
               std::make_pair("filler3"sv, 16),
               std::make_pair("filler4"sv, 17),
               std::make_pair("filler5"sv, 148),
               std::make_pair("account-greeks"sv, 5),
               std::make_pair("orders"sv, 2),
               std::make_pair("orders-algo"sv, 1),
               std::make_pair("algo-advance"sv, 121),
               std::make_pair("filler6"sv, 53),
               std::make_pair("filler7"sv, 42),
               std::make_pair("filler8"sv, 12),
               std::make_pair("filler9"sv, 44),
               std::make_pair("filler10"sv, 64),
               std::make_pair("filler11"sv, 55)
);

static inline constexpr auto apply23 = prs::details::makeFirstMatch(5,
               std::make_pair("positions"sv, 1),
               std::make_pair("balance_and_position"sv, 67),
               std::make_pair("liquidation-warning"sv, 11),
               std::make_pair("filler1"sv, 14),
               std::make_pair("filler2"sv, 115),
               std::make_pair("filler3"sv, 16),
               std::make_pair("filler4"sv, 17),
               std::make_pair("filler5"sv, 148),
               std::make_pair("account-greeks"sv, 5),
               std::make_pair("orders"sv, 2),
               std::make_pair("orders-algo"sv, 1),
               std::make_pair("algo-advance"sv, 121),
               std::make_pair("filler6"sv, 53),
               std::make_pair("filler7"sv, 42),
               std::make_pair("filler8"sv, 12),
               std::make_pair("filler9"sv, 44),
               std::make_pair("filler10"sv, 64),
               std::make_pair("filler11"sv, 55),
               std::make_pair("filler12"sv, 3456),
               std::make_pair("filler13"sv, 432),
               std::make_pair("filler14"sv, 1245),
               std::make_pair("filler15"sv, 75),
               std::make_pair("filler16"sv, 78)
);


static inline const std::map<std::string_view, int> map4 = {
        {"positions"sv, 1},
        {"balance_and_position"sv, 67},
        {"liquidation-warning"sv, 11},
        {"account-greeks"sv, 5},
};

static inline const std::map<std::string_view, int> map7 = {
        {"positions"sv, 1},
        {"balance_and_position"sv, 67},
        {"liquidation-warning"sv, 11},
        {"account-greeks"sv, 5},
        {"orders"sv, 2},
        {"orders-algo"sv, 1},
        {"algo-advance"sv, 12}
};

static inline const std::map<std::string_view, int> map12 = {
        {"positions"sv, 1},
        {"balance_and_position"sv, 67},
        {"liquidation-warning"sv, 11},
        {"filler1"sv, 14},
        {"filler2"sv, 115},
        {"filler3"sv, 16},
        {"filler4"sv, 17},
        {"filler5"sv, 148},
        {"account-greeks"sv, 5},
        {"orders"sv, 2},
        {"orders-algo"sv, 1},
        {"algo-advance"sv, 12}
};


static inline const std::unordered_map<std::string_view, int> map18 = {
        {"positions"sv, 1},
        {"balance_and_position"sv, 67},
        {"liquidation-warning"sv, 11},
        {"filler1"sv, 14},
        {"filler2"sv, 115},
        {"filler3"sv, 16},
        {"filler4"sv, 17},
        {"filler5"sv, 148},
        {"account-greeks"sv, 5},
        {"orders"sv, 2},
        {"orders-algo"sv, 1},
        {"algo-advance"sv, 12},
        {"filler6"sv, 53},
        {"filler7"sv, 42},
        {"filler8"sv, 12},
        {"filler9"sv, 44},
        {"filler10"sv, 64},
        {"filler11"sv, 55}
};


static inline const std::unordered_map<std::string_view, int> map23 = {
        {"positions"sv, 1},
        {"balance_and_position"sv, 67},
        {"liquidation-warning"sv, 11},
        {"filler1"sv, 14},
        {"filler2"sv, 115},
        {"filler3"sv, 16},
        {"filler4"sv, 17},
        {"filler5"sv, 148},
        {"account-greeks"sv, 5},
        {"orders"sv, 2},
        {"orders-algo"sv, 1},
        {"algo-advance"sv, 12},
        {"filler6"sv, 53},
        {"filler7"sv, 42},
        {"filler8"sv, 12},
        {"filler9"sv, 44},
        {"filler10"sv, 64},
        {"filler11"sv, 55},
        {"filler12"sv, 3456},
        {"filler13"sv, 432},
        {"filler14"sv, 1245},
        {"filler15"sv, 75},
        {"filler16"sv, 7}
};


static inline const std::unordered_map<std::string_view, int> umap4 = {
        {"positions"sv, 1},
        {"balance_and_position"sv, 67},
        {"liquidation-warning"sv, 11},
        {"account-greeks"sv, 5},
};

static inline const std::unordered_map<std::string_view, int> umap7 = {
        {"positions"sv, 1},
        {"balance_and_position"sv, 67},
        {"liquidation-warning"sv, 11},
        {"account-greeks"sv, 5},
        {"orders"sv, 2},
        {"orders-algo"sv, 1},
        {"algo-advance"sv, 12}
};

static inline const std::unordered_map<std::string_view, int> umap12 = {
        {"positions"sv, 1},
        {"balance_and_position"sv, 67},
        {"liquidation-warning"sv, 11},
        {"filler1"sv, 14},
        {"filler2"sv, 115},
        {"filler3"sv, 16},
        {"filler4"sv, 17},
        {"filler5"sv, 148},
        {"account-greeks"sv, 5},
        {"orders"sv, 2},
        {"orders-algo"sv, 1},
        {"algo-advance"sv, 12}
};


static inline const std::unordered_map<std::string_view, int> umap18 = {
        {"positions"sv, 1},
        {"balance_and_position"sv, 67},
        {"liquidation-warning"sv, 11},
        {"filler1"sv, 14},
        {"filler2"sv, 115},
        {"filler3"sv, 16},
        {"filler4"sv, 17},
        {"filler5"sv, 148},
        {"account-greeks"sv, 5},
        {"orders"sv, 2},
        {"orders-algo"sv, 1},
        {"algo-advance"sv, 12},
        {"filler6"sv, 53},
        {"filler7"sv, 42},
        {"filler8"sv, 12},
        {"filler9"sv, 44},
        {"filler10"sv, 64},
        {"filler11"sv, 55}
};


static inline const std::unordered_map<std::string_view, int> umap23 = {
        {"positions"sv, 1},
        {"balance_and_position"sv, 67},
        {"liquidation-warning"sv, 11},
        {"filler1"sv, 14},
        {"filler2"sv, 115},
        {"filler3"sv, 16},
        {"filler4"sv, 17},
        {"filler5"sv, 148},
        {"account-greeks"sv, 5},
        {"orders"sv, 2},
        {"orders-algo"sv, 1},
        {"algo-advance"sv, 12},
        {"filler6"sv, 53},
        {"filler7"sv, 42},
        {"filler8"sv, 12},
        {"filler9"sv, 44},
        {"filler10"sv, 64},
        {"filler11"sv, 55},
        {"filler12"sv, 3456},
        {"filler13"sv, 432},
        {"filler14"sv, 1245},
        {"filler15"sv, 75},
        {"filler16"sv, 7}
};


#define GEN_TEST_FOR(n) \
static inline auto challengeW ## n = generateChallengeSamplesWithWeights(n); \
BENCHMARK_CAPTURE(BM_ApplyFirstMatchStrings, Apply ## n ## Weights, apply ## n, challengeW4, n);\
BENCHMARK_CAPTURE(BM_ApplyFirstMatchMapStrings, ApplyMap ## n ## Weights, map ## n, challengeW4, n);\
BENCHMARK_CAPTURE(BM_ApplyFirstMatchMapStrings, ApplyUMap ## n ## Weights, umap ## n, challengeW4, n); \
                        \
static inline auto challengeR ## n = generateChallengeSamplesWithoutWeights(n);\
BENCHMARK_CAPTURE(BM_ApplyFirstMatchStrings, Apply ## n ## Random, apply ## n, challengeR4, n);\
BENCHMARK_CAPTURE(BM_ApplyFirstMatchMapStrings, ApplyMap ## n ## Random, map ## n, challengeR4, n);\
BENCHMARK_CAPTURE(BM_ApplyFirstMatchMapStrings, ApplyUMap ## n ## Random, umap ## n, challengeR4, n);\

#ifdef ENABLE_HARD_BENCHMARK
GEN_TEST_FOR(4);
GEN_TEST_FOR(7);
GEN_TEST_FOR(12);
GEN_TEST_FOR(18);
GEN_TEST_FOR(23);
#else
GEN_TEST_FOR(12);
#endif