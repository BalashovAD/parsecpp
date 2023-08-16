#include "../testHelper.h"

TEST(Process, ProcessRepeatUnit) {
    std::string s;
    auto parser = charFrom(FromRange('a', 'z')) * processRepeat([&](char c) {
        s += toupper(c);
    });

    success_parsing(parser, Unit{}, "zaqA", "A");
    EXPECT_EQ(s, "ZAQ");
    s = "";
    success_parsing(parser, Unit{}, "1zaqA", "1zaqA");
    EXPECT_EQ(s, "");
}


struct RepeatMap : public Repeat<RepeatMap, std::map<char, unsigned>, VoidContext> {
    void add(Container& map, char c) const noexcept {
        ++map[c];
    }
};

TEST(Process, ProcessRepeatMap) {
    auto parser = charFrom(FromRange('a', 'z')) * RepeatMap{};

    success_parsing(parser, {{'a', 2}, {'q', 1}, {'z', 1}}, "zaaqA", "A");
    success_parsing(parser, {}, "1zaqA", "1zaqA");
}

TEST(Process, ConvertResult) {
    auto parser = until('|') * convertResult(number());

    success_parsing(parser, 124, "124|A", "|A");
    success_parsing(parser, 12, "12p4|A", "|A"); // p4 won't be parsed at all

    failed_parsing(parser, 0, "tt|tt");
}


TEST(Process, ConvertResultFull) {
    auto parser = until('|') * convertResult(number().endOfStream());

    success_parsing(parser, 124, "124|A", "|A");

    failed_parsing(parser, 2, "12p4|A");
    failed_parsing(parser, 0, "tt|tt");
}


TEST(Process, searchWord) {
    auto parser = search(lettersFrom(FromRange('a', 'z'), FromRange('A', 'Z')).mustConsume()).repeat().mustConsume();

    success_parsing(parser, {"test", "t", "test", "q"}, "test t test  12q 11", " 11");
    success_parsing(parser, {"a"}, "123a123", "123");

    failed_parsing(parser, 0, "");
    failed_parsing(parser, 0, "12");
}

TEST(Process, searchRollback) {
    auto parser = search(literal("aaa") >> literal("bbb"));
    success_parsing(parser, "bbb", "aaaabbb");
    failed_parsing(parser, 0, "aaaaaa");
    failed_parsing(parser, 0, "aaacbbb");
}

TEST(Process, searchRollback2) {
    auto parser = search(literal("aaa")).repeat().mustConsume();
    success_parsing(parser, {"aaa"}, "aaaabbb", "abbb");
    success_parsing(parser, {"aaa", "aaa"}, "aaaaaa");
    success_parsing(parser, {"aaa", "aaa"}, "aaacaaadaa", "daa");
    failed_parsing(parser, 0, "aa");
}

TEST(Process, searchRollback3) {
    auto parser = search(literal("aaa")).maybe() >> literal("bbb");
    success_parsing(parser, {"bbb"}, "aaabbb", "");
    success_parsing(parser, {"bbb"}, "bbb", "");

    failed_parsing(parser, 0, "aabbb");
    failed_parsing(parser, 3, "aaaabbb");
    failed_parsing(parser, 4, "baaabb");
}

TEST(Process, searchAlwaysPossessive) {
    // this search always failed, because `repeat` take 'b'
    auto parser = search(charFrom<'a', 'b'>().repeat() >> literal<"bc"_prs>());
    failed_parsing(parser, 0, "abababc");
    failed_parsing(parser, 0, "abc");
    failed_parsing(parser, 0, "bc");
}
