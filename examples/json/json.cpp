#include <parsecpp/full.hpp>

#include <iostream>
#include <memory>
#include <iomanip>
#include <variant>
#include <vector>
#include <map>


using namespace prs;

class Json {
public:
    struct Null{};
    using ptr = std::shared_ptr<Json>;

    using Variant = std::variant<
            double,
            std::string,
            bool,
            Null,
            std::vector<Json::ptr>,
            std::map<std::string, Json::ptr>>;

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

        std::string operator()(std::map<std::string, Json::ptr> const& map) const noexcept {
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


        std::string operator()(std::string const& s) const noexcept {
            return "\"" + s + "\"";
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

using PJ = Parser<Json::ptr, debug::DebugContext>;
using Make = details::MakeShared<Json>;

PJ parseObject() noexcept;
PJ parseArray() noexcept;


auto parseString() noexcept {
    return (between<std::string>('"') *= debug::SaveParsedSource{"String"}) >>= Make{};
}

auto parseDouble() noexcept {
    return (number<double>() *= debug::SaveParsedSource{"Number"}) >>= Make{};
}

auto parseUndefined() noexcept {
    return (spaces() >> literal("null") >> spaces() >> pure(Json::Null{}) *= debug::SaveParsedSource{"Undefined"}) >>= Make{};
}

auto parseBool() noexcept {
    return (spaces() >> (literal("false") >> pure(false) | literal("true") >> pure(true)) *= debug::SaveParsedSource{"Boolean"}) >>= Make{};
}

auto parseAny() noexcept {
    return lazy(parseObject) | lazy(parseArray) | parseString() | parseDouble() | parseUndefined() | parseBool();
}

PJ parseObject() noexcept {
    auto parserPre = charFromSpaces('{');
    auto parserKey = spaces() >> between<std::string>('"') << charFromSpaces(':');
    auto parserDelim = charFromSpaces(',');
    auto parserPost = charFromSpaces('}');
    return (parserPre >>
            (toMap(parserKey, parseAny(), parserDelim) >>= Make{})
        << parserPost << debug::logPoint("Object end") *= debug::AddStackLevel{}).toCommonType();
}

PJ parseArray() noexcept {
    auto parserPre = charFromSpaces('[');
    auto parserDelim = charFromSpaces(',');
    auto parserPost = charFromSpaces(']');
    return (parserPre >>
         (parseAny().repeat(parserDelim) >>= Make{})
        << parserPost << debug::logPoint("Array end") *= debug::AddStackLevel{}).toCommonType();
}


int main() {
    std::string strJson;

    auto parser = parseAny().endOfStream();

    // {"a": "test", "b"  : 33 , "c":[1,2,{"q": 2}, 4], "d": null, "eee": {"r":false}}
    getline(std::cin, strJson);
    Stream stream{strJson};
    debug::DebugContext ctx;
    Finally printStack([&]() {
        std::cout << "\n" << ctx.get().print(stream) << std::endl;
    });

    return parser(stream, ctx).join([&](Json::ptr const& spExpr) {
        std::cout << "Json: " << spExpr->toString() << std::endl;
        return 0;
    }, [&](details::ParsingError const& error) {
        std::cout << stream.generateErrorText(error) << std::endl;
        return 1;
    });
}