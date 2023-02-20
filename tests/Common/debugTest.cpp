#include "../testHelper.h"

TEST(Debug, LogPoint) {
    debug::DebugEnvironment env;
    auto parser = number<unsigned>() << debug::logPoint(env, "test") << charIn('a');
    Stream stream{"12345a"};
    EXPECT_FALSE(parser(stream).isError());
    std::cout << env.print(stream) << std::endl;
}


TEST(Debug, ParserWorkSuccess) {
    debug::DebugEnvironment env;
    auto parser = spaces() >> debug::parserWork(env, number<int>(), "Int") << spaces();
    Stream stream{" 12345a"};
    EXPECT_FALSE(parser(stream).isError());
    std::cout << env.print(stream) << std::endl;
}

TEST(Debug, ParserWorkError) {
    debug::DebugEnvironment env;
    auto parser = spaces() >> debug::parserWork(env, number<int>(), "Int").maybe() << spaces();
    Stream stream{"  a"};
    EXPECT_FALSE(parser(stream).isError());
    std::cout << env.print(stream) << std::endl;
}

TEST(Debug, ParserError) {
    debug::DebugEnvironment env;
    auto parser = spaces() >> debug::parserError(env, number<int>(), "Int").maybe() << spaces();
    Stream stream{"  a"};
    EXPECT_FALSE(parser(stream).isError());
    std::cout << env.print(stream) << std::endl;
}