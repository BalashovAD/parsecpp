#include "../testHelper.h"


Parser<char> makeB();

auto makeA() {
    return charFrom('A') >> makeB();
}

Parser<char> makeB() {
    return (charFrom('B') | lazy(makeA)).toCommonType();
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
    return (concat(charFrom('(', '{', '['), lazy(braces).maybe() >> charFrom(')', '}', ']'))
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

Parser<Unit> bracesCached() noexcept {
    return (concat(charFrom('(', '{', '['), lazyCached(bracesCached, AutoTagV).maybe() >> charFrom(')', '}', ']'))
        .cond([](std::tuple<char, char> const& cc) {
            return details::cmpAnyOf(cc
                     , std::make_tuple('(', ')')
                     , std::make_tuple('{', '}')
                     , std::make_tuple('[', ']'));
    }).repeat<5>() >> pure(Unit{})).toCommonType();
}

TEST(Lazy, BracesCached) {
    auto parser = bracesCached().endOfStream();

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


struct ATag;
auto makeBCached() -> Parser<char>;

auto makeACached() {
    return charFrom('A') >> makeBCached();
}


Parser<char> makeBCached() {
    return (charFrom('B') | lazyCached<AutoTagT>(makeACached)).toCommonType();
}


TEST(LazyCached, AB) {
    auto parser = makeACached();
    success_parsing(parser, 'B', "ABABA", "ABA");
    success_parsing(parser, 'B', "AAAAAB", "");
    success_parsing(parser, 'B', "ABBBB", "BBB");

    failed_parsing(parser, 0, "BBBA");
    failed_parsing(parser, 4, "AAAAC");
}

using TagForForget = AutoTagT;
auto f() noexcept -> decltype((charFrom('a') >> std::declval<Parser<Unit, VoidContext, LazyForget<Unit, TagForForget>>>() >> charFrom('b')).maybe() >> success()) {
    return (charFrom('a') >> lazyForget<Unit, TagForForget>(f) >> charFrom('b')).maybe() >> success();
}

TEST(LazyForget, rec) {
    auto parser = f().endOfStream();
    success_parsing(parser, Unit{}, "aaabbb", "");
    success_parsing(parser, Unit{}, "ab", "");
    success_parsing(parser, Unit{}, "", "");

    failed_parsing(parser, 2, "abab");
    failed_parsing(parser, 0, "aaabb");
    failed_parsing(parser, 6, "aaabbbb");
}


Parser<char> parserWithAutoTag() {
    return (charFrom('a') << lazyCached(parserWithAutoTag, AutoTagV).maybe()).toCommonType();
}

Parser<char> parserWithAutoTag2() {
    return (charFrom('b') << lazyCached(parserWithAutoTag2, AutoTagV).maybe()).toCommonType();
}

TEST(AutoTag, Test) {
    constexpr auto hash1 = AutoTagT::value;
    constexpr auto hash2 = AutoTagT::value;

    static_assert(hash1 != hash2);
}

TEST(LazyCached, AutoTag) {
    auto parserA = parserWithAutoTag();
    auto parserB = parserWithAutoTag2();

    success_parsing(parserA, 'a', "aaa");
    success_parsing(parserB, 'b', "baa", "aa");
}


struct NonUniqueTag;
Parser<char> parserWithOneTag() {
    return (charFrom('a') << lazyCached<NonUniqueTag>(parserWithOneTag).maybe()).toCommonType();
}

Parser<char> parserWithOneTag2() {
    return (charFrom('b') << lazyCached<NonUniqueTag>(parserWithOneTag).maybe()).toCommonType();
}

TEST(LazyCached, NonUniqueTag) {
    auto parserA = parserWithOneTag();
    auto parserB = parserWithOneTag2();

    success_parsing(parserA, 'a', "aaa");
    success_parsing(parserB, 'b', "baa", "");
    success_parsing(parserB, 'b', "bbb", "bb");

    failed_parsing(parserB, 0, "aaa");
}