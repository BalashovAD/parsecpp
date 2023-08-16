#include "../testHelper.h"

#include <cmath>

TEST(Number, ParseUnsigned) {
    auto parser = number<unsigned>();

    success_parsing(parser, 4u, "4", "");
    success_parsing(parser, 4123u, "4123", "");

    using NumberType = GetParserResult<decltype(parser)>;
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


TEST(Number, ParseDouble) {
    auto parser = number<double>();

    success_parsing(parser, 4., "4", "");
    success_parsing(parser, 4123., "4123", "");

    double max = std::numeric_limits<double>::max();
    success_parsing(parser, max, std::to_string(max), "");
    success_parsing(parser, 0., "0", "");

    success_parsing(parser, 4.1, "4.1 ", " ");
    success_parsing(parser, .1, "0.1a", "a");
    success_parsing(parser, -4., "-4");


    {
        Stream nan{"NaNa"};
        auto result = parser(nan);
        ASSERT_FALSE(result.isError());
        EXPECT_TRUE(std::isnan(result.data()));
    }

    failed_parsing(parser, 0, "a");
    failed_parsing(parser, 0, " ");
    failed_parsing(parser, 0, "");
}

TEST(Number, Digits) {
    auto parser = digits();
    success_parsing(parser, "234", "234a", "a");
    success_parsing(parser, "1", "1", "");

    // no length boundaries
    success_parsing(parser, "123456789012345678901234567890", "123456789012345678901234567890x", "x");

    // always success
    success_parsing(parser, "", "x", "x");
    success_parsing(parser, "", "", "");
}

TEST(Number, Digit) {
    auto parser = digit();
    success_parsing(parser, '2', "234a", "34a");
    success_parsing(parser, '1', "1", "");

    failed_parsing(parser, 0, "abc");
    failed_parsing(parser, 0, "");
}
