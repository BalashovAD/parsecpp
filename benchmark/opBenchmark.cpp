#include <benchmark/benchmark.h>

#include <parsecpp/all.h>

using namespace prs;

static constexpr std::string_view OP_TEST = "  ttt 123 rrr222   ttt4444 o3 q1 eeeeeee                         1234567 y 5";

static void BM_OpSimpleTest(benchmark::State& state) {
    auto parser = (spaces() >> letters() >> spaces() >> number<unsigned>()).repeat<false, 10>();
    for (auto _ : state) {
        Stream s(OP_TEST);
        auto data = parser(s);
        benchmark::DoNotOptimize(data.data().size());
    }
}

BENCHMARK(BM_OpSimpleTest);

static void BM_ETALON_OpSimpleTest(benchmark::State& state) {
    for (auto _ : state) {
        std::vector<unsigned> data;

        std::string_view s = OP_TEST;

        size_t i = 0;
        while (s[i]) {
            char c = s[i];
            while (std::isspace(c) && s[i]) {
                c = s[++i];
            }

            size_t keyStart = i;
            while (('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z') && s[i]) {
                c = s[++i];
            }

            while (std::isspace(c) && s[i]) {
                c = s[++i];
            }

            unsigned value;
            auto result = std::from_chars(s.data() + i, s.end(), value);

            i = result.ptr - s.data();

            data.emplace_back(value);
        }

        benchmark::DoNotOptimize(data.size());
    }
}

BENCHMARK(BM_ETALON_OpSimpleTest);

static constexpr std::string_view EMPTY_OP_TEST = "bigwordbigwordbigwordbigword";
static void BM_EmptyOp(benchmark::State& state) {
    auto parser = spaces() >> letters() << spaces();
    for (auto _ : state) {
        Stream s(EMPTY_OP_TEST);
        auto data = parser(s);
        benchmark::DoNotOptimize(data.data().size());
    }
}

BENCHMARK(BM_EmptyOp);


static void BM_EmptyOpFastSpaces(benchmark::State& state) {
    auto parser = spacesFast() >> letters() << spacesFast();
    for (auto _ : state) {
        Stream s(EMPTY_OP_TEST);
        auto data = parser(s);
        benchmark::DoNotOptimize(data.data().size());
    }
}

BENCHMARK(BM_EmptyOpFastSpaces);

static void BM_EmptyOpSuccess(benchmark::State& state) {
    auto parser = success() >> letters() << success();
    for (auto _ : state) {
        Stream s(EMPTY_OP_TEST);
        auto data = parser(s);
        benchmark::DoNotOptimize(data.data().size());
    }
}

BENCHMARK(BM_EmptyOpSuccess);


static void BM_ETALON_EmptyOp(benchmark::State& state) {
    auto parser = letters();
    for (auto _ : state) {
        Stream s(EMPTY_OP_TEST);
        auto data = parser(s);
        benchmark::DoNotOptimize(data.data().size());
    }
}

BENCHMARK(BM_ETALON_EmptyOp);

static constexpr std::string_view TYPE_ERASING_OP_TEST = "testtest test";
static void BM_TypeErasing(benchmark::State& state) {
    auto parser = letters().toCommonType();
    for (auto _ : state) {
        Stream s(TYPE_ERASING_OP_TEST);
        auto data = parser(s);
        benchmark::DoNotOptimize(data.data().size());
    }
}

BENCHMARK(BM_TypeErasing);

static void BM_ETALON_TypeErasing(benchmark::State& state) {
    auto parser = letters();
    for (auto _ : state) {
        Stream s(TYPE_ERASING_OP_TEST);
        auto data = parser(s);
        benchmark::DoNotOptimize(data.data().size());
    }
}

BENCHMARK(BM_ETALON_TypeErasing);