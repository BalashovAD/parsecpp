#include "../testHelper.h"


TEST(HitCounter, Common) {
    ContextWrapper<HitCounterType<>> ctx{0};
    auto parser = (charFrom('a', 'b') >> charFrom('c') >> hitCounter()).drop().repeat();
    success_parsing(parser, {}, "acbcacacc", "c", ctx);
    EXPECT_EQ(ctx.get(), 4);
}


TEST(HitCounter, ContinueCounting) {
    unsigned n = 0;
    ContextWrapper<HitCounterType<>> ctx{n};
    auto parser = (charFrom('a', 'b') >> charFrom('c') >> hitCounter()).drop().repeat();
    success_parsing(parser, {}, "acbcacacc", "c", ctx);
    EXPECT_EQ(ctx.get(), 4);
    success_parsing(parser, {}, "acbcacacc", "c", ctx);
    EXPECT_EQ(ctx.get(), 8);
}

TEST(HitCounter, Revert) {
    unsigned n = 0;
    ContextWrapper<HitCounterType<>> ctx{n};
    auto parser = (charFrom('a', 'b') << hitCounter() << charFrom('c')).repeat();
    success_parsing(parser, {'a', 'b', 'a'}, "acbcaca", "a", ctx);
    EXPECT_EQ(ctx.get(), 4);
}

TEST(HitCounter, FailParser) {
    unsigned n = 0;
    ContextWrapper<HitCounterType<>> ctx{n};
    auto parser = (charFrom('a', 'b') >> hitCounter() >> charFrom('c'));
    failed_parsing(parser, 1, "ad", ctx);
    EXPECT_EQ(ctx.get(), 1);
}