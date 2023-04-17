#include "../testHelper.h"


TEST(Exception, Common) {
    auto parser = (charFrom('a') >> exceptionParser() << literal("test")).mustConsume().repeat().endOfStream();

    expect_throw(parser, "atest", "t");
    success_parsing(parser, {}, "");
    failed_parsing(parser, 0, "ttt");
}

TEST(Exception, Combo) {
    auto parser = (charFrom('a') >> (exceptionParser().maybe() >>= details::Id{}) << literal("test")).mustConsume().repeat().endOfStream();

    expect_throw(parser, "atest", "t");
    success_parsing(parser, {}, "");
    failed_parsing(parser, 0, "ttt");
}