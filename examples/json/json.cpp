#include <parsecpp/all.h>
#include <parsecpp/utils/applyFirstMatch.h>

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
            return s;
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
    return between<std::string>('"') >>= Make{};
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

auto parseAny() noexcept {
    return lazy(parseObject) | lazy(parseArray) | parseString() | parseDouble() | parseUndefined() | parseBool();
}

PJ parseObject() noexcept {
    auto parserPre = charInSpaces('{');
    auto parserKey = spaces() >> between<std::string>('"') << charInSpaces(':');
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
         (toArray(parseAny(), parserDelim) >>= Make{})
        << parserPost).toCommonType();
}


int main() {
    std::string strJson;

    auto parser = parseAny();

    // {"a":"test", "b"  : 33 , "c":[1,2,3,4], "d": null, "eee": {"r":false}}
    getline(std::cin, strJson);
    Stream stream{strJson};
    return parser(stream).join([](Json::ptr const& spExpr) {
        std::cout << "Json: " << spExpr->toString() << std::endl;
        return 0;
    }, [&](details::ParsingError const& error) {
        std::cout << stream.generateErrorText(error) << std::endl;
        return 1;
    });
}