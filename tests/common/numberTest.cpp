#include "../testHelper.h"

#include <cmath>

TEST(Base, ParseUnsigned) {
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


TEST(Base, ParseDouble) {
    auto parser = number<double>();

    success_parsing(parser, 4., "4", "");
    success_parsing(parser, 4123., "4123", "");

    double max = std::numeric_limits<double>::max();
    success_parsing(parser, max, std::to_string(max), "");
    success_parsing(parser, 0., "0", "");

    success_parsing(parser, 4.1, "4.1 ", " ");
    success_parsing(parser, .1, "0.1a", "a");
    success_parsing(parser, 4., "+4");


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