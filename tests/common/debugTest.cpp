#include "../testHelper.h"

TEST(Debug, LogPoint) {
    debug::DebugEnvironment env;
    debug::DebugContext ctx{env};

    auto parser = number<unsigned>() << debug::logPoint("test") << charFrom('a');
    Stream stream{"12345a"};

    EXPECT_FALSE(parser(stream, ctx).isError());
    std::cout << env.print(stream) << std::endl;
}


TEST(Debug, ParserWorkSuccess) {
    debug::DebugEnvironment env;
    debug::DebugContext ctx{env};

    auto parser = spaces() >> debug::parserWork(number<int>(), "Int") << spaces();
    Stream stream{" 12345a"};
    EXPECT_FALSE(parser(stream, ctx).isError());
    std::cout << env.print(stream) << std::endl;
}

TEST(Debug, ParserWorkError) {
    debug::DebugEnvironment env;
    debug::DebugContext ctx{env};

    auto parser = spaces() >> debug::parserWork(number<int>(), "Int").maybe() << spaces();
    Stream stream{"  a"};
    EXPECT_FALSE(parser(stream, ctx).isError());
    std::cout << env.print(stream) << std::endl;
}

TEST(Debug, ParserError) {
    debug::DebugEnvironment env;
    debug::DebugContext ctx{env};

    auto parser = spaces() >> debug::parserError(number<int>(), "Int").maybe() << spaces();
    Stream stream{"  a"};
    EXPECT_FALSE(parser(stream, ctx).isError());
    std::cout << env.print(stream) << std::endl;
}

TEST(Debug, DoubleDebugContext) {
    debug::DebugEnvironment env;
    debug::DebugContext ctx{env};

    auto parser = charFrom('a') >>
            number<unsigned>() * ModifyWithContext<debug::ParserWork<false>, debug::DebugContext>{"Number"}
        << charFrom('b') << debug::LogPoint{"end"}.toParser();
    Stream stream{"a123bc"};
    EXPECT_FALSE(parser(stream, ctx).isError());
    std::cout << env.print(stream) << std::endl;
}