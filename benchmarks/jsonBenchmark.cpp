#include <benchmark/benchmark.h>

#include <parsecpp/full.hpp>

#include <fstream>
#include <variant>
#include <iostream>


using namespace prs;


class Json {
public:
    struct Null{};
    using ptr = std::shared_ptr<Json>;

    using Variant = std::variant<
            double,
            std::string_view,
            bool,
            Null,
            std::vector<Json::ptr>,
            std::map<std::string_view, Json::ptr>>;

    struct ToString {
        std::string operator()(Null) const noexcept {
            return "null";
        }

        std::string operator()(std::vector<Json::ptr> const& arr) const noexcept {
            std::ostringstream os;
            os << "[";
            bool isFirst = true;
            for (const auto& spJson : arr) {
                if (!isFirst) {
                    os << ",";
                }
                os << spJson->toString();
                isFirst = false;
            }
            os << "]";
            return os.str();
        }

        std::string operator()(std::map<std::string_view, Json::ptr> const& map) const noexcept {
            std::ostringstream os;
            os << "{";
            bool isFirst = true;
            for (const auto& [key, spJson] : map) {
                if (!isFirst) {
                    os << ",";
                }
                os << "\"" << key << "\":" << spJson->toString();
                isFirst = false;
            }
            os << "}";
            return os.str();
        }


        std::string operator()(std::string_view const& s) const noexcept {
            return std::string(s);
        }

        std::string operator()(double t) const noexcept {
            return std::to_string(t);
        }

        std::string operator()(bool t) const noexcept {
            return t ? "true" : "false";
        }
    };


    template <typename T>
    requires (std::is_constructible_v<Variant, T>)
    explicit Json(T const& t) noexcept
        : m_value(t) {

    }

    std::string toString() const noexcept {
        return std::visit(ToString{}, m_value);
    }
private:
    Variant m_value;
};

using PJ = Parser<Json::ptr>;
using Make = details::MakeShared<Json>;

PJ parseObject() noexcept;
PJ parseArray() noexcept;


auto parseString() noexcept {
    return between('"') >>= Make{};
}

auto parseDouble() noexcept {
    return number() >>= Make{};
}

auto parseUndefined() noexcept {
    return (spaces() >> literal("null") >> spaces() >> pure(Json::Null{})) >>= Make{};
}

auto parseBool() noexcept {
    return (spaces() >> (literal("false") >> pure(false)) | (literal("true") >> pure(true))) >>= Make{};
}
struct ObjectTag;
struct ArrayTag;
auto parseAny() noexcept {
    return lazyCached<ObjectTag>(parseObject)
            | lazyCached<ArrayTag>(parseArray)
            | parseString()
            | parseDouble()
            | parseUndefined()
            | parseBool();
}

PJ parseObject() noexcept {
    auto parserPre = charFromSpaces('{');
    auto parserKey = spaces() >> between('"') << charFromSpaces(':');
    auto parserDelim = charFromSpaces(',');
    auto parserPost = charFromSpaces('}');
    return (parserPre >>
                      (toMap(parserKey, parseAny(), parserDelim) >>= Make{})
                      << parserPost).toCommonType();
}


PJ parseArray() noexcept {
    auto parserPre = charFromSpaces('[');
    auto parserDelim = charFromSpaces(',');
    auto parserPost = charFromSpaces(']');
    return (parserPre >>
          (parseAny().repeat<10>(parserDelim) >>= Make{})
          << parserPost).toCommonType();
}

static void BM_jsonFile(benchmark::State& state, std::string filename) {
    auto parser = parseAny();
    std::ifstream file{filename};
    if (!file.is_open()) {
        state.SkipWithError("Cannot open file");
        return;
    }
    std::string json{std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
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

BENCHMARK_CAPTURE(BM_jsonFile, 100k, "100k.json");
BENCHMARK_CAPTURE(BM_jsonFile, canada, "canada.json");
BENCHMARK_CAPTURE(BM_jsonFile, binance, "binance.json");

#ifdef ENABLE_HARD_BENCHMARK

BENCHMARK_CAPTURE(BM_jsonFile, 64kb, "64kb.json");
BENCHMARK_CAPTURE(BM_jsonFile, 64kb_min, "64kb-min.json");

BENCHMARK_CAPTURE(BM_jsonFile, 256kb, "256kb.json");
BENCHMARK_CAPTURE(BM_jsonFile, 256kb_min, "256kb-min.json");

BENCHMARK_CAPTURE(BM_jsonFile, 5mb, "5mb.json");
BENCHMARK_CAPTURE(BM_jsonFile, 5mb_min, "5mb-min.json");

#endif


struct BinanceTrade {
    size_t id;
    double price;
    double qty;
    size_t time;
    bool isBuyerMaker;
};



template <typename Parser>
static void BM_jsonSpecializedBinance(benchmark::State& state, Parser parser) {

    std::ifstream file{"./binance.json"};
    if (!file.is_open()) {
        state.SkipWithError("Cannot open file");
        return;
    }
    std::string json{std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};

    for (auto _ : state) {
        Stream s(json);
        auto data = parser(s);
        if (data.isError()) {
            std::cout << s.generateErrorText(data.error()) << std::endl;
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
