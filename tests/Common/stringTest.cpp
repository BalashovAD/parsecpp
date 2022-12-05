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