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

TEST(String, lettersFromConstexpr) {
    auto parser = lettersFrom<FromRange('a', 'z'), 'A', 'B', 'C'>();

    success_parsing(parser, "test", "test", "");
    success_parsing(parser, "tAesCt", "tAesCt", "");
    success_parsing(parser, "", "Ztest", "Ztest");
    success_parsing(parser, "test", "test test", " test");
}

TEST(String, skipChars) {
    auto parser = skipChars(FromRange('a', 'z'), 'A', 'B', 'C');

    success_parsing(parser, {}, "test", "");
    success_parsing(parser, {}, "tZtest", "Ztest");
    success_parsing(parser, {}, "Ztest", "Ztest");
    success_parsing(parser, {}, "", "");
}

TEST(String, skipCharsConstexpr) {
    auto parser = skipChars<FromRange('a', 'z'), 'A', 'B', 'C'>();

    success_parsing(parser, {}, "test", "");
    success_parsing(parser, {}, "tZtest", "Ztest");
    success_parsing(parser, {}, "Ztest", "Ztest");
    success_parsing(parser, {}, "", "");
}

TEST(String, until) {
    auto parser = until(FromRange('a', 'z'), 'A', 'B', 'C');

    success_parsing(parser, "Z", "Ztest", "test");
    success_parsing(parser, "", "test", "test");
    success_parsing(parser, "WER", "WER", "");
}

TEST(String, untilAnySpace) {
    auto parser = until<AnySpace{}>();

    success_parsing(parser, "Z", "Z test", " test");
    success_parsing(parser, "", "\ttest", "\ttest");
    success_parsing(parser, "1", "1\ntest", "\ntest");
    success_parsing(parser, "WER", "WER", "");
}

TEST(String, untilConstexpr) {
    auto parser = until<FromRange('a', 'z'), 'A', 'B', 'C'>();

    success_parsing(parser, "Z", "Ztest", "test");
    success_parsing(parser, "", "test", "test");
    success_parsing(parser, "WER", "WER", "");
}

TEST(String, untilFnDoubleSymbol) {
    auto parser = until([](char c, Stream& s) {
        return s.sv().size() > 1 && s.remaining()[1] == c;
    });

    success_parsing(parser, "test", "test", "");
    success_parsing(parser, "a", "abbc", "bbc");
    success_parsing(parser, "", "bbc", "bbc");
    success_parsing(parser, "a", "abb", "bb");
}


TEST(String, escapedString) {
    auto parser = charFrom<'"'>() >> escapedString<'"', '\\'>();

    success_parsing(parser, "test", R"("test")");
    success_parsing(parser, R"(te"st)", R"("te\"st")");
    success_parsing(parser, R"(te\"st)", R"("te\\\"st")");
    success_parsing(parser, R"(te\)", R"("te\\"st")", R"(st")");

    failed_parsing(parser, 1, R"("test)");
    failed_parsing(parser, 1, R"("test\")");
    failed_parsing(parser, 1, R"("test\\\")");
    failed_parsing(parser, 1, R"(")");
}

TEST(String, escapedStringSame) {
    auto parser = charFrom('"') >> escapedString<'"', '"'>();

    success_parsing(parser, "test", R"("test")");
    success_parsing(parser, R"(te"st)", R"("te""st")");
    success_parsing(parser, R"(te""st)", R"("te""""st")");
    success_parsing(parser, R"(te")", R"("te"""st")", R"(st")");
    success_parsing(parser, R"(Super, "luxurious" truck)", R"("Super, ""luxurious"" truck")", R"()");

    failed_parsing(parser, 1, R"("test)");
    failed_parsing(parser, 1, R"("test"")");
    failed_parsing(parser, 1, R"("test"""")");
    failed_parsing(parser, 1, R"(")");
}
