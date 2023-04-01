#include "../testHelper.h"


TEST(HitCounter, Common) {
    unsigned n = 0;
    ContextWrapper<HitCounterType<>> ctx{n};
    auto parser = (charFrom('a', 'b') >> charFrom('c') >> hitCounter()).drop().repeat();
    success_parsing(parser, {}, "acbcacacc", "c", ctx);
    EXPECT_EQ(n, 4);
}


TEST(HitCounter, ContinueCounting) {
    unsigned n = 0;
    ContextWrapper<HitCounterType<>> ctx{n};
    auto parser = (charFrom('a', 'b') >> charFrom('c') >> hitCounter()).drop().repeat();
    success_parsing(parser, {}, "acbcacacc", "c", ctx);
    EXPECT_EQ(n, 4);
    success_parsing(parser, {}, "acbcacacc", "c", ctx);
    EXPECT_EQ(n, 8);
}

TEST(HitCounter, Revert) {
    unsigned n = 0;
    ContextWrapper<HitCounterType<>> ctx{n};
    auto parser = (charFrom('a', 'b') << hitCounter() << charFrom('c')).repeat();
    success_parsing(parser, {'a', 'b', 'a'}, "acbcaca", "a", ctx);
    EXPECT_EQ(n, 4);
}

TEST(HitCounter, FailParser) {
    unsigned n = 0;
    ContextWrapper<HitCounterType<>> ctx{n};
    auto parser = (charFrom('a', 'b') >> hitCounter() >> charFrom('c'));
    failed_parsing(parser, 1, "ad", ctx);
    EXPECT_EQ(n, 1);
}