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