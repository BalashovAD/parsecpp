#include "../testHelper.h"

TEST(String, between) {
    auto parser = between('(', ')');

    success_parsing(parser, "test", "(test)", "");
    success_parsing(parser, "test", "(test)test", "test");
    success_parsing(parser, "", "())", ")");
    success_parsing(parser, "", "()", "");

    failed_parsing(parser, 0, " (test)");
    failed_parsing(parser, 5, "(test");
    failed_parsing(parser, 6, "(test(");
    failed_parsing(parser, 0, ")test(");
    failed_parsing(parser, 0, "");
    failed_parsing(parser, 0, "test");
}

TEST(String, betweenSame) {
    auto parser = between('*');

    success_parsing(parser, "test", "*test*", "");
    success_parsing(parser, "test", "*test*test", "test");
    success_parsing(parser, "", "***", "*");
    success_parsing(parser, "", "**", "");

    failed_parsing(parser, 0, " *test*");
    failed_parsing(parser, 5, "*test");
    failed_parsing(parser, 0, "");
    failed_parsing(parser, 0, "test");
}

TEST(String, lettersFrom) {
    auto parser = lettersFrom(FromRange('a', 'z'), 'A', 'B', 'C');

    success_parsing(parser, "test", "test", "");
    success_parsing(parser, "tAesCt", "tAesCt", "");
    success_parsing(parser, "", "Ztest", "Ztest");
    success_parsing(parser, "test", "test test", " test");
}

TEST(String, until) {
    auto parser = until(FromRange('a', 'z'), 'A', 'B', 'C');

    success_parsing(parser, "Z", "Ztest", "test");
    success_parsing(parser, "", "test", "test");
    success_parsing(parser, "WER", "WER", "");
}

TEST(String, untilFnDoubleSymbol) {
    auto parser = until([](char c, Stream& s) {
        return !s.eos() && s.remaining()[1] == c;
    });

    success_parsing(parser, "test", "test", "");
    success_parsing(parser, "a", "abbc", "bbc");
    success_parsing(parser, "", "bbc", "bbc");
    success_parsing(parser, "a", "abb", "bb");
}


TEST(String, searchWord) {
    auto parser = search(lettersFrom(FromRange('a', 'z'), FromRange('A', 'Z')).mustConsume()).repeat().mustConsume();

    success_parsing(parser, {"test", "t", "test", "q"}, "test t test  12q 11", " 11");
    success_parsing(parser, {"a"}, "123a123", "123");

    failed_parsing(parser, 0, "");
    failed_parsing(parser, 0, "12");
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

    Color(StrHex tr, StrHex tg, StrHex tb) noexcept
        : r(f(tr))
        , g(f(tg))
        , b(f(tb)) {

    }

    bool operator==(Color const& c) const noexcept {
        return std::tie(r, g, b) == std::tie(c.r, c.g, c.b);
    }

    unsigned r, g, b;
};

TEST(String, searchHex) {

    auto hexParser = charFrom(FromRange('0', '9'), FromRange('a', 'f'), FromRange('A', 'F'));
    auto hexNumberParser = concat(hexParser, hexParser);
    auto parser = search(
            charFrom('#').maybe() >>
            liftM(details::MakeClass<Color>{}, hexNumberParser, hexNumberParser, hexNumberParser)
            << charFrom(';').maybe()).repeat();

    success_parsing(parser, {Color{0xF0, 0xF0, 0xF0}, Color{0xFA, 0xFA, 0xFA}, Color{0xFF, 0xF0, 0x00}},
R"(HEX code with numbers in it: #F0F0F0

HEX code with letters only: #FAFAFA;

Works without a # in front of the code: FFF000;.)", ".");
}