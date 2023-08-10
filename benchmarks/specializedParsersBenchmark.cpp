#include <benchmark/benchmark.h>

#include <parsecpp/full.hpp>

#include "benchmarkHelper.hpp"

#include <iostream>


using namespace prs;

struct BinanceTrade {
    size_t id;
    double price;
    double qty;
    size_t time;
    bool isBuyerMaker;
};


template <typename Parser>
static void BM_jsonSpecializedBinance(benchmark::State& state, Parser parser) {

    std::string json = readFile("./binance.json");
    for (auto _ : state) {
        Stream s(json);
        auto data = parser(s);
        if (data.isError()) {
            state.SkipWithError("Cannot parse json");
        } else {
            if (data.data().size() != 1000) {
                state.SkipWithError("Not full parsed");
            }
        }
        benchmark::DoNotOptimize(data.data());
    }

    state.SetBytesProcessed(json.size() * state.iterations());
}


auto jsonValueDouble(std::string fieldName) noexcept {
    return searchText("\"" + fieldName + "\":") >> charFrom('"') >> number<double>() << charFrom('"');
}

auto jsonValueUnsigned(std::string fieldName) noexcept {
    return searchText("\"" + fieldName + "\":") >> number<size_t>();
}

auto binanceParser() {
    return charFrom('[') >> (charFrom('{') >> liftM(details::MakeClass<BinanceTrade>{},
            jsonValueUnsigned("id"),
            jsonValueDouble("price"),
            jsonValueDouble("qty"),
            jsonValueUnsigned("time"),
            searchText("\"isBuyerMaker\":") >> (literal("true") >> pure(true) | pure(false))
    ) << searchText("}") << charFrom(',').maybe()).repeat<1000>() << charFrom(']');
}


template <ConstexprString fieldName>
auto jsonValueDoubleConstexpr() noexcept {
    return searchText<fieldName.between('"').add(':')>() >> charFrom<'"'>() >> number<double>() << charFrom<'"'>();
}

template <ConstexprString fieldName>
auto jsonValueUnsignedConstexpr() noexcept {
    return searchText<fieldName.between('"').add(':')>() >> number<size_t>();
}

using std::string_view_literals::operator""sv;

static constexpr auto boolFromString = details::makeFirstMatch(false,
        std::make_pair("true"sv, true),
        std::make_pair("false"sv, false));

auto binanceParserConstexpr() {
    return charFrom<'['>() >> (charFrom<'{'>() >> liftM(details::MakeClass<BinanceTrade>{},
            jsonValueUnsignedConstexpr<"id"_prs>(),
            jsonValueDoubleConstexpr<"price"_prs>(),
            jsonValueDoubleConstexpr<"qty"_prs>(),
            jsonValueUnsignedConstexpr<"time"_prs>(),
            searchText<"isBuyerMaker"_prs.between('"').add(':')>() >> (letters() >>= boolFromString)
    ) << searchText<"}"_prs>() << charFrom<','>().maybe()).repeat<1000>() << charFrom<']'>();
}


BENCHMARK_CAPTURE(BM_jsonSpecializedBinance, Common, binanceParser());
BENCHMARK_CAPTURE(BM_jsonSpecializedBinance, TypeErasing, binanceParser().toCommonType());
BENCHMARK_CAPTURE(BM_jsonSpecializedBinance, Constexpr, binanceParserConstexpr());
