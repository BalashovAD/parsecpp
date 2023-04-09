#include "../testHelper.h"

static constexpr auto a = charFrom('a');
static constexpr auto b = charFrom('b');
static constexpr auto c = charFrom('c');
//static constexpr auto d = charFrom('d');

char f(char q) noexcept {
    return toupper(q);
}

template <char q>
struct Mod {
    auto operator()(ModifyCallerI<char>& parser, Stream& s) const {
        return parser().map([](char value) {
            if (value == q) {
                return '!';
            } else {
                return value;
            }
        });
    }
};

// a >> b | c === (a >> b) | c
TEST(OpPriority, ForgetOr) {
    auto parser = a >> b | c;

    success_parsing(parser, 'c', "cab", "ab");

    failed_parsing(parser, 1, "acd");
}


// a >> b | c >>= F === ((a >> b) | c) >>= F
TEST(OpPriority, ForgetOrFmap) {
    auto parser = a >> b | c >>= f;

    success_parsing(parser, 'C', "cab", "ab");
    success_parsing(parser, 'B', "abd", "d");

    failed_parsing(parser, 1, "acd");
}


// a << b * Mod === (a << (b * Mod))
TEST(OpPriority, ForgetMod) {
    auto parser = a << b * Mod<'a'>{};
    success_parsing(parser, 'a', "abd", "d"); // mod changes only b

    auto parser2 = (a << b) * Mod<'a'>{};
    success_parsing(parser2, '!', "abd", "d");
}

// a << b *= Mod === (a << b) * Mod
TEST(OpPriority, ForgetModLow) {
    auto parser = a << b *= Mod<'a'>{};
    success_parsing(parser, '!', "abd", "d");
}