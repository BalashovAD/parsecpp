#include <benchmark/benchmark.h>

#include <parsecpp/all.h>

#include <iostream>


using namespace prs;

static constexpr std::string_view ERROR_A_LOT_OF_TIMES = "abc bbb333 v123 d e2";

static void BM_ErrorHandling(benchmark::State& state) {
    auto parser = (letters<false, false>() >> number<int>().maybe() << spaces()).repeat<6>().cond([](auto const& t) {
        return t.size() > 5;
    });

    for (auto _ : state) {
        Stream s(ERROR_A_LOT_OF_TIMES);
        auto data = parser(s);
        if (!data.isError()) {
            std::cout << "Error parsed" << std::endl;
        }
    }
}

BENCHMARK(BM_ErrorHandling);