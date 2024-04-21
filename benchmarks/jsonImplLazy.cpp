#include <parsecpp/full.hpp>

#include <variant>

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

using namespace prs;

using PJ = Parser<Json::ptr>;
using Make = details::MakeShared<Json>;

inline auto parseString() noexcept {
    return between('"') >>= Make{};
}

inline auto parseDouble() noexcept {
    return number() >>= Make{};
}

inline auto parseUndefined() noexcept {
    return (spaces() >> literal("null") >> spaces() >> pure(Json::Null{})) >>= Make{};
}

inline auto parseBool() noexcept {
    return (spaces() >> (literal("false") >> pure(false)) | (literal("true") >> pure(true))) >>= Make{};
}


namespace implLazy {

PJ parseObject() noexcept;
PJ parseArray() noexcept;

inline auto parseAny() noexcept {
    return lazy(parseObject)
           | lazy(parseArray)
           | parseString()
           | parseDouble()
           | parseUndefined()
           | parseBool();
}

inline PJ parseObject() noexcept {
    auto parserPre = charFromSpaces('{');
    auto parserKey = spaces() >> between('"') << charFromSpaces(':');
    auto parserDelim = charFromSpaces(',');
    auto parserPost = charFromSpaces('}');
    return (parserPre >>
                      (toMap(parserKey, parseAny(), parserDelim) >>= Make{})
                      << parserPost).toCommonType();
}


inline PJ parseArray() noexcept {
    auto parserPre = charFromSpaces('[');
    auto parserDelim = charFromSpaces(',');
    auto parserPost = charFromSpaces(']');
    return (parserPre >>
                      (parseAny().repeat<10>(parserDelim) >>= Make{})
                      << parserPost).toCommonType();
}

}


namespace implLazyCached {

PJ parseObject() noexcept;
PJ parseArray() noexcept;

struct ObjectTag;
struct ArrayTag;

inline auto parseAny() noexcept {
    return lazyCached<ObjectTag>(parseObject)
           | lazyCached<ArrayTag>(parseArray)
           | parseString()
           | parseDouble()
           | parseUndefined()
           | parseBool();
}


inline PJ parseObject() noexcept {
    auto parserPre = charFromSpaces('{');
    auto parserKey = spaces() >> between('"') << charFromSpaces(':');
    auto parserDelim = charFromSpaces(',');
    auto parserPost = charFromSpaces('}');
    return (parserPre >>
                      (toMap(parserKey, parseAny(), parserDelim) >>= Make{})
                      << parserPost).toCommonType();
}


inline PJ parseArray() noexcept {
    auto parserPre = charFromSpaces('[');
    auto parserDelim = charFromSpaces(',');
    auto parserPost = charFromSpaces(']');
    return (parserPre >>
                      (parseAny().repeat<10>(parserDelim) >>= Make{})
                      << parserPost).toCommonType();
}

}


namespace implSelfLazy {


inline auto parseObject(auto const& anyParser) noexcept {
    auto parserPre = charFromSpaces('{');
    auto parserKey = spaces() >> between('"') << charFromSpaces(':');
    auto parserDelim = charFromSpaces(',');
    auto parserPost = charFromSpaces('}');
    return (parserPre >>
                      (toMap(parserKey, anyParser, parserDelim) >>= Make{})
                      << parserPost);
}


inline auto parseArray(auto const& anyParser) noexcept {
    auto parserPre = charFromSpaces('[');
    auto parserDelim = charFromSpaces(',');
    auto parserPost = charFromSpaces(']');
    return (parserPre >>
                      (anyParser.template repeat<10>(parserDelim) >>= Make{})
                      << parserPost);
}

static inline auto parseBoolV = parseBool();
static inline auto parseUndefinedV = parseUndefined();
static inline auto parseDoubleV = parseDouble();
static inline auto parseStringV = parseString();

inline auto parseAny() noexcept {
    return selfLazy<Json::ptr>([](auto const& p) {
        return parseObject(p)
               | parseArray(p)
               | parseStringV
               | parseDoubleV
               | parseUndefinedV
               | parseBoolV;
    });
}

}