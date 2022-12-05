#include "../testHelper.h"


TEST(Base, ParseNumber) {
    auto parser = number<unsigned>();

    success_parsing(parser, 4u, "4", "");
    success_parsing(parser, 4123u, "4123", "");

    using NumberType = parser_result_t<decltype(parser)>;
    NumberType max = std::numeric_limits<NumberType>::max();
    success_parsing(parser, max, std::to_string(max), "");
    success_parsing(parser, 0u, "0", "");

    success_parsing(parser, 4u, "4 ", " ");
    success_parsing(parser, 4u, "4a", "a");

    failed_parsing(parser, 0, "a");
    failed_parsing(parser, 0, " ");
    failed_parsing(parser, 0, "");
    failed_parsing(parser, 0, "44444444444444444444444");
    failed_parsing(parser, 0, std::to_string(max) + "0");
    failed_parsing(parser, 0, "+4");
}

TEST(Base, ParseSpaces) {
    auto parser = spaces();

    success_parsing(parser, Unit{}, " 4", "4");
    success_parsing(parser, Unit{}, "    4", "4");
    success_parsing(parser, Unit{}, "0", "0");
    success_parsing(parser, Unit{}, "", "");
}


TEST(Base, ParseWord) {
    auto parser = letters();

    success_parsing(parser, "test", "test", "");
    success_parsing(parser, "test", "test w", " w");
    success_parsing(parser, "test", "test2", "2");
    success_parsing(parser, "", " test", " test");
}

TEST(Base, ParseWordNoEmpty) {
    auto parser = letters<false>();

    success_parsing(parser, "test", "test", "");

    failed_parsing(parser, 0, "");
    failed_parsing(parser, 0, " test");
    failed_parsing(parser, 0, "2test");
}

TEST(Base, ParseWordAllowDigit) {
    auto parser = letters<true, true>();

    success_parsing(parser, "test2", "test2", "");
    success_parsing(parser, "2test", "2test", "");
}

TEST(Base, SearchText) {
    auto parser = searchText("test");

    success_parsing(parser, Unit{}, "test2", "2");
    success_parsing(parser, Unit{}, "2test", "");
    success_parsing(parser, Unit{}, "2test2", "2");
    success_parsing(parser, Unit{}, "2testtest2", "test2");

    failed_parsing(parser, 0, "te st");
    failed_parsing(parser, 0, "TeSt");
    failed_parsing(parser, 0, "");
}