#include <benchmark/benchmark.h>

#include <parsecpp/all.hpp>

#include <variant>
#include <iostream>


using namespace prs;

template <size_t reserve = 0, typename ParserValue>
auto constructParserInLoopRepeat(ParserValue value) noexcept {
    using Value = GetParserResult<ParserValue>;
    using P = Parser<std::vector<Value>>;
    return P::make([value](Stream& stream, auto& ctx) {
        std::vector<Value> ans{};

        ans.reserve(reserve);

        auto backup = stream.pos();

        const auto emplace = [&](auto value) {
            backup = stream.pos();
            ans.emplace_back(std::move(value));
            return Drop{};
        };

        auto parser = value >>= emplace;
        for (bool isError = parser.apply(stream, ctx).isError();
             !isError;
             isError = parser.apply(stream, ctx).isError());

        stream.restorePos(backup);
        return P::data(ans);
    });
}


template <size_t reserve = 0, typename ParserValue>
auto doWhile(ParserValue value) noexcept {
    using Value = GetParserResult<ParserValue>;
    using Vector = std::vector<Value>;
    return Parser<Vector>::make([parser = value](Stream& stream, auto& ctx) {
        Vector out;
        out.reserve(reserve);

        size_t iteration = 0;
        auto backup = stream.pos();
        do {
            auto result = parser.apply(stream, ctx);
            if (!result.isError()) {
                out.emplace_back(std::move(result).data());
                backup = stream.pos();
            } else {
                return Parser<Vector>::data(std::move(out));
            }
        } while (++iteration != 10000000ull);

        return Parser<Vector>::makeError("Max iteration in repeat", stream.pos());
    });
}

template <size_t reserve = 0, typename ParserValue>
auto doWhileNoCtx(ParserValue value) noexcept {
    using Value = GetParserResult<ParserValue>;
    using Vector = std::vector<Value>;
    return Parser<Vector>::make([parser = value](Stream& stream) {
        Vector out;
        out.reserve(reserve);

        size_t iteration = 0;
        auto backup = stream.pos();
        do {
            auto result = parser.apply(stream);
            if (!result.isError()) {
                out.emplace_back(std::move(result).data());
                backup = stream.pos();
            } else {
                return Parser<Vector>::data(std::move(out));
            }
        } while (++iteration != 10000000ull);

        return Parser<Vector>::makeError("Max iteration in repeat", stream.pos());
    });
}

template <size_t reserve = 0, typename ParserValue>
auto Ycomb(ParserValue value) noexcept {
    using Value = GetParserResult<ParserValue>;
    using Vector = std::vector<Value>;
    return Parser<Vector>::make([parser = value](Stream& stream, auto& ctx) {
        std::vector<Value> ans{};
        ans.reserve(reserve);

        auto backup = stream.pos();

        const auto emplace = [&](auto value) {
            backup = stream.pos();
            ans.emplace_back(std::move(value));
            return Drop{};
        };

        auto np = (parser >>= emplace);
        const auto p = [&](auto const& rec) -> void {
            np.apply(stream, ctx).map([&](Drop _) {
                backup = stream.pos();
                rec();
                return _;
            });
        };

        details::Y{p}();
        stream.restorePos(backup);
        return Parser<Vector>::data(ans);
    });
}


template <ParserType ParserT, size_t reserve = 0>
class DoWhileTest {
public:
    using T = GetParserResult<ParserT>;
    using Vector = std::vector<T>;
    using ReturnParser = Parser<Vector>;

    static auto makeEmplace(std::shared_ptr<Vector>& spAns) noexcept {
        spAns = std::make_shared<Vector>();
        return [&ans = *spAns](T value) {
            ans.emplace_back(std::move(value));
            return Drop{};
        };
    }

    explicit DoWhileTest(ParserT p) noexcept
        : m_parser(std::move(p) >>= makeEmplace(m_spAns)) {
    }

    typename ReturnParser::Result operator()(Stream& stream) const noexcept(ParserT::nothrow) {
        size_t backup{};
        auto& ans = *m_spAns;
        ans.reserve(reserve);
        do {
            backup = stream.pos();
        } while (!m_parser.apply(stream).isError());

        stream.restorePos(backup);
        return ReturnParser::data(std::move(ans));
    }
private:
    std::shared_ptr<Vector> m_spAns;
    decltype(std::declval<ParserT>() >>= makeEmplace(std::declval<std::shared_ptr<Vector>&>())) m_parser;
};


template <size_t reserve = 0, typename ParserValue>
auto doWhileClass(ParserValue value) noexcept {
    return DoWhileTest<ParserValue, reserve>::ReturnParser::make(DoWhileTest<ParserValue, reserve>{std::move(value)});
}


std::string generateChallenge(size_t size) noexcept {
    std::stringstream s;
    const auto genWord = [](size_t i) {
        int letters = (i * i * 13 + 7 * i + 3) % 16 + 8;
        std::string out;
        for (auto it = 0; it != letters; ++it) {
            out += (char)('a' + it);
        }
        return out;
    };
    for (size_t i = 0; i != size; ++i) {
        s << '{' << genWord(i) << '}';
    }
    std::cout << "C: " << s.str() << std::endl;
    return s.str();
}

static inline std::string CHALLENGE = generateChallenge(100);


template <ParserType Parser>
static void BM_toArray(benchmark::State& state, Parser const& parser) {
    for (auto _ : state) {
        Stream s{CHALLENGE};

        auto ans = parser.apply(s);
        if (ans.isError()) {
            throw std::runtime_error("Cannot parse");
        } else {
            if (ans.data().size() != 100) {
                throw std::runtime_error("size");
            }
        }
    }
}

static inline auto insideParser = charFrom('{') >> letters<false, std::string>() << charFrom('}');

BENCHMARK_CAPTURE(BM_toArray, Loop, constructParserInLoopRepeat(insideParser));
BENCHMARK_CAPTURE(BM_toArray, DoWhile, doWhile(insideParser));
BENCHMARK_CAPTURE(BM_toArray, DoWhileNoCtx, doWhileNoCtx(insideParser));
BENCHMARK_CAPTURE(BM_toArray, Ycomb, Ycomb(insideParser));
BENCHMARK_CAPTURE(BM_toArray, Class, doWhileClass(insideParser));

BENCHMARK_CAPTURE(BM_toArray, ReserveLoop, constructParserInLoopRepeat<100>(insideParser));
BENCHMARK_CAPTURE(BM_toArray, ReserveDoWhile, doWhile<100>(insideParser));
BENCHMARK_CAPTURE(BM_toArray, ReserveDoWhileNoCtx, doWhileNoCtx<100>(insideParser));
BENCHMARK_CAPTURE(BM_toArray, ReserveYcomb, Ycomb<100>(insideParser));
BENCHMARK_CAPTURE(BM_toArray, ReserveClass, doWhileClass<100>(insideParser));
