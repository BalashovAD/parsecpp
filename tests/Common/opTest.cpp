#include "../testHelper.h"

TEST(Op, ForgetLeft) {
    auto parser = letters<false>() >> number<unsigned>();

    success_parsing(parser, 4u, "a4", "");
    success_parsing(parser, 444u, "test444test", "test");

    failed_parsing(parser, 4, "test");
    failed_parsing(parser, 0, "444");
    failed_parsing(parser, 0, "444");
}


TEST(Op, ForgetRight) {
    auto parser = letters() << searchText("test");

    success_parsing(parser, "a", "a4test", "");
    success_parsing(parser, "a", "a4testt", "t");
    success_parsing(parser, "test", "test test", "");

    failed_parsing(parser, 4, "test");
    failed_parsing(parser, 0, "444");
}

TEST(Op, Maybe) {
    auto parser = number<unsigned>().maybe();

    success_parsing(parser, 4, "4test", "test");
    success_parsing(parser, std::nullopt, "a4testt", "a4testt");
    success_parsing(parser, std::nullopt, "", "");
}

TEST(Op, MaybeErrorHandling) {
    auto parser = (charFrom('1') >> charFrom('2')).maybe() >> charFrom('1');

    success_parsing(parser, '1', "13", "3");
}

TEST(Op, Repeat) {
    auto parser = anyChar().repeat<0, 10>().cond([](auto const& t) {
        return !t.empty();
    });

    success_parsing(parser, {'4', 't', 'e', 's', 't'}, "4test", "");
    success_parsing(parser, {'4'}, "4", "");

    failed_parsing(parser, 0, "");
    failed_parsing(parser, 10, "xxxxxxxxxxxxxxxxxxxxxxx");
}

TEST(Op, RepeatDrop) {
    auto parser = anyChar().drop().repeat<10>();

    success_parsing(parser, {}, "4test", "");
    success_parsing(parser, {}, "4", "");
    success_parsing(parser, {}, "");

    failed_parsing(parser, 10, "xxxxxxxxxxxxxxxxxxxxxxx");
}

TEST(Op, Or) {
    auto parser = (number<unsigned>() >>= details::ToString{}) | letters<true, std::string>();

    success_parsing(parser, "4", "4test", "test");
    success_parsing(parser, "test", "test", "");
    success_parsing(parser, "4444", "4444", "");

    failed_parsing(parser, 0, "");
    failed_parsing(parser, 0, "###");
}

TEST(Op, Concat) {
    auto parser = concat(number<unsigned>(), spaces() >> letters<false>());

    success_parsing(parser, std::make_tuple(4, "test"), "4test", "");
    success_parsing(parser, std::make_tuple(4, "test"), "4 test", "");
    success_parsing(parser, std::make_tuple(4, "test"), "4    test", "");
    success_parsing(parser, std::make_tuple(4, "test"), "4    test444", "444");

    failed_parsing(parser, 0, "");
    failed_parsing(parser, 1, "4");
    failed_parsing(parser, 1, "4*");
}


TEST(Op, LiftM) {
    auto parser = liftM([](unsigned n, char c) {
        return std::to_string(n) + c;
    }, number<unsigned>(), spaces() >> charFrom('a', 'b'));

    success_parsing(parser, "4b", "4    btest444", "test444");

    failed_parsing(parser, 0, "c");
    failed_parsing(parser, 0, "");
    failed_parsing(parser, 1, "4*");
}

