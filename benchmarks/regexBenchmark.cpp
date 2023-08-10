#include <benchmark/benchmark.h>

#include <parsecpp/all.hpp>

#include "benchmarkHelper.hpp"

#include <variant>
#include <iostream>
#include <regex>
#include <fstream>


using namespace prs;


template <ParserType P>
void BM_SearchSuccess(benchmark::State& state, P parser, std::string const& filename) {

    std::string test = readFile(filename);
    for (auto _ : state) {
        Stream s{test};
        auto result = parser(s);
        if (result.isError()) {
            state.SkipWithError("Cannot parse");
        }
    }

    state.SetBytesProcessed(test.size() * state.iterations());
}


struct Color {
    static constexpr int ASCIIHexToInt[] =
            {
                    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
                    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
                    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
                    0,  1,  2,  3,  4,  5,  6,  7,  8,  9, -1, -1, -1, -1, -1, -1,
                    -1, 10, 11, 12, 13, 14, 15, -1, -1, -1, -1, -1, -1, -1, -1, -1,
                    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
                    -1, 10, 11, 12, 13, 14, 15, -1, -1, -1, -1, -1, -1, -1, -1, -1,
                    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
            };


    using StrHex = std::tuple<char, char>;

    constexpr static unsigned f(StrHex s) noexcept {
        return ASCIIHexToInt[static_cast<unsigned char>(get<0>(s))] * 16 + ASCIIHexToInt[static_cast<unsigned char>(get<1>(s))];
    }

    Color(unsigned rr, unsigned gg, unsigned bb) noexcept
        : r(rr)
        , g(gg)
        , b(bb) {

    }

    Color(StrHex tr, StrHex tg, StrHex tb, StrHex ta) noexcept
        : r(f(tr))
        , g(f(tg))
        , b(f(tb))
        , a(f(ta)) {

    }

    bool operator==(Color const& c) const noexcept {
        return std::tie(r, g, b) == std::tie(c.r, c.g, c.b);
    }

    unsigned r, g, b, a;
};

auto colorParser() noexcept {
    auto hexParser = charFrom(FromRange('0', '9'), FromRange('a', 'f'), FromRange('A', 'F'));
    auto hexNumberParser = concat(hexParser, hexParser);
    return search(
            charFrom('#').maybe() >>
            liftM(details::MakeClass<Color>{}
                    , hexNumberParser
                    , hexNumberParser
                    , hexNumberParser
                    , hexNumberParser.maybeOr(std::make_tuple('0', '0')))
            << charFrom(';').maybe()).repeat();
}

BENCHMARK_CAPTURE(BM_SearchSuccess, HexColor, colorParser(), "./colors.css");

void BM_RegexHexColor(benchmark::State& state) {
    std::regex regex{R"##(#?([\da-fA-F]{2})([\da-fA-F]{2})([\da-fA-F]{2})([\da-fA-F]{2})?)##"};
    std::string test = readFile("./colors.css");

    for (auto _ : state) {
        std::vector<Color> data;
        std::string_view s{test};
        std::cmatch match;
        while (std::regex_search(s.begin(), s.end(), match, regex)) {
            if (match.size() >= 4) {
                data.emplace_back(std::tie(match[1].first[0], match[1].first[1])
                        , std::tie(match[2].first[0], match[2].first[1])
                        , std::tie(match[3].first[0], match[3].first[1])
                        , match[4].matched ? std::make_tuple(match[4].first[0], match[4].first[1]) : std::make_tuple('0', '0'));
            } else {
                throw std::runtime_error("match");
            }
            s.remove_prefix(match.position() + match.length());
        }
    }
    state.SetBytesProcessed(test.size() * state.iterations());
}

BENCHMARK(BM_RegexHexColor);

