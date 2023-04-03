#include "../testHelper.h"

TEST(Debug, LogPoint) {
    debug::DebugContext ctx{{}};

    auto parser = number<unsigned>() << debug::logPoint("test") << charFrom('a');
    Stream stream{"12345a"};

    EXPECT_FALSE(parser(stream, ctx).isError());
//    std::cout << ctx.get().print(stream) << std::endl;
    EXPECT_FALSE(ctx.get().print(stream).empty());
}


TEST(Debug, ParserWorkSuccess) {
    debug::DebugContext ctx{{}};

    auto parser = spaces() >> debug::parserWork(number<int>(), "Int") << spaces();
    Stream stream{" 12345a"};
    EXPECT_FALSE(parser(stream, ctx).isError());
//    std::cout << ctx.get().print(stream) << std::endl;
    EXPECT_FALSE(ctx.get().print(stream).empty());
}

TEST(Debug, ParserWorkError) {
    debug::DebugContext ctx{{}};

    auto parser = spaces() >> debug::parserWork(number<int>(), "Int").maybe() << spaces();
    Stream stream{"  a"};
    EXPECT_FALSE(parser(stream, ctx).isError());
//    std::cout << ctx.get().print(stream) << std::endl;
    EXPECT_FALSE(ctx.get().print(stream).empty());
}

TEST(Debug, ParserError) {
    debug::DebugContext ctx{};

    auto parser = spaces() >> debug::parserError(number<int>(), "Int").maybe() << spaces();
    Stream stream{"  a"};
    EXPECT_FALSE(parser(stream, ctx).isError());
//    std::cout << ctx.get().print(stream) << std::endl;
    EXPECT_FALSE(ctx.get().print(stream).empty());
}

TEST(Debug, DoubleDebugContext) {
    debug::DebugContext ctx{};

    auto parser = charFrom('a') >>
            number<unsigned>() * ModifyWithContext<debug::ParserWork<false>, debug::DebugContext>{"Number"}
        << charFrom('b') << debug::LogPoint{"end"}.toParser();
    Stream stream{"a123bc"};
    EXPECT_FALSE(parser(stream, ctx).isError());
//    std::cout << ctx.get().print(stream) << std::endl;
    EXPECT_FALSE(ctx.get().print(stream).empty());
}