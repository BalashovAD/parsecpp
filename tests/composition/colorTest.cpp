#include "../testHelper.h"


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
