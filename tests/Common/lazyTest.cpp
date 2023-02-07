#include "../testHelper.h"


Parser<char> makeB();

auto makeA() {
    return charIn('A') >> makeB();
}

Parser<char> makeB() {
    return (charIn('B') | lazy(makeA)).toCommonType();
}

TEST(Lazy, ABA) {
    auto parser = makeA();
    success_parsing(parser, 'B', "ABABA", "ABA");
    success_parsing(parser, 'B', "AAAAAB", "");
    success_parsing(parser, 'B', "ABBBB", "BBB");

    failed_parsing(parser, 0, "BBBA");
    failed_parsing(parser, 4, "AAAAC");
}

Parser<Unit> braces() noexcept {
    return (concat(charIn('(', '{', '['), lazy(braces).maybe() >> charIn(')', '}', ']'))
        .cond([](std::tuple<char, char> const& cc) {
            return details::cmpAnyOf(cc
                     , std::make_tuple('(', ')')
                     , std::make_tuple('{', '}')
                     , std::make_tuple('[', ']'));
    }).repeat() >> pure(Unit{})).toCommonType();
}

TEST(Lazy, Braces) {
    auto parser = braces().endOfStream();

    success_parsing(parser, Unit{}, "()", "");
    success_parsing(parser, Unit{}, "{}", "");
    success_parsing(parser, Unit{}, "[]", "");

    failed_parsing(parser, 0, "{)");
    failed_parsing(parser, 0, "}()");
    failed_parsing(parser, 2, "{} ");
    failed_parsing(parser, 0, "(()");
}

TEST(Lazy, BracesRec) {
    auto parser = braces().endOfStream();

    success_parsing(parser, Unit{}, "({[]})");
    success_parsing(parser, Unit{}, "{()()()}");
    success_parsing(parser, Unit{}, "{}()[]");
    success_parsing(parser, Unit{}, "");

    failed_parsing(parser, 0, "(((]]]");
    failed_parsing(parser, 0, "({)}");
}

Parser<char> makeBCached();

auto makeACached() {
    return charIn('A') >> makeBCached();
}
struct ATag{};
Parser<char> makeBCached() {
    return (charIn('B') | lazyCached<ATag>(makeACached)).toCommonType();
}


TEST(LazyCached, AB) {
    auto parser = makeACached();
    success_parsing(parser, 'B', "ABABA", "ABA");
    success_parsing(parser, 'B', "AAAAAB", "");
    success_parsing(parser, 'B', "ABBBB", "BBB");

    failed_parsing(parser, 0, "BBBA");
    failed_parsing(parser, 4, "AAAAC");
}
