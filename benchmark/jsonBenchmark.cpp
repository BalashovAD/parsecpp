#include <benchmark/benchmark.h>

#include <parsecpp/all.h>

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
    auto parserPre = charInSpaces('{');
    auto parserKey = spaces() >> between('"') << charInSpaces(':');
    auto parserDelim = charInSpaces(',');
    auto parserPost = charInSpaces('}');
    return (parserPre >>
                      (toMap(parserKey, parseAny(), parserDelim) >>= Make{})
                      << parserPost).toCommonType();
}


PJ parseArray() noexcept {
    auto parserPre = charInSpaces('[');
    auto parserDelim = charInSpaces(',');
    auto parserPost = charInSpaces(']');
    return (parserPre >>
          (toArray<10>(parseAny(), parserDelim) >>= Make{})
          << parserPost).toCommonType();
}

static void BM_json100k(benchmark::State& state) {
    auto parser = parseAny();
    std::ifstream file{"./100k.json"};
    std::string json{std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};

    for (auto _ : state) {
        Stream s(json);
        auto data = parser(s);
        if (data.isError()) {
            throw std::runtime_error("Cannot parse json");
        }
        benchmark::DoNotOptimize(data.data());
    }
}

BENCHMARK(BM_json100k);


static void BM_citmCatalog(benchmark::State& state) {
    auto parser = parseAny();
    std::ifstream file{"./canada.json"};
    std::string json{std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};

    for (auto _ : state) {
        Stream s(json);
        auto data = parser(s);
        if (data.isError()) {
            std::cout << "Error: " << s.generateErrorText<10, 5>(data.error()) << std::endl;
            throw std::runtime_error("Cannot parse json");
        }
        benchmark::DoNotOptimize(data.data());
    }
}


BENCHMARK(BM_citmCatalog);

static void BM_jsonBinance(benchmark::State& state) {
    auto parser = parseAny();
    std::ifstream file{"./binance.json"};
    std::string json{std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};

    for (auto _ : state) {
        Stream s(json);
        auto data = parser(s);
        if (data.isError()) {
            throw std::runtime_error("Cannot parse json");
        }
        benchmark::DoNotOptimize(data.data());
    }
}


BENCHMARK(BM_jsonBinance);


auto jsonValueDouble(std::string fieldName) noexcept {
    return searchText("\"" + fieldName + "\":") >> charIn('"') >> number<double>() << charIn('"');
}

auto jsonValueUnsigned(std::string fieldName) noexcept {
    return searchText("\"" + fieldName + "\":") >> number<size_t>();
}

static void BM_jsonSpecializedBinance(benchmark::State& state) {
    struct BinanceTrade {
        size_t id;
        double price;
        double qty;
        size_t time;
        bool isBuyerMaker;
    };

    auto parser = charIn('[') >> (charIn('{') >> liftM(details::MakeClass<BinanceTrade>{},
            jsonValueUnsigned("id"),
            jsonValueDouble("price"),
            jsonValueDouble("qty"),
            jsonValueUnsigned("time"),
            searchText("\"isBuyerMaker\":") >> (literal("true") >> pure(true) | pure(false))
        ) << searchText("}") << charIn(',').maybe()).repeat<true, 500>() << charIn(']');

    std::ifstream file{"./binance.json"};
    std::string json{std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};

    for (auto _ : state) {
        Stream s(json);
        auto data = parser(s);
        if (data.isError()) {
            std::cout << s.generateErrorText(data.error()) << std::endl;
            throw std::runtime_error("Cannot parse json");
        } else {
            if (data.data().size() != 1000) {
                throw std::runtime_error("Not full parsed: " + std::to_string(data.data().size()));
            }
        }
        benchmark::DoNotOptimize(data.data());
    }
}


BENCHMARK(BM_jsonSpecializedBinance);