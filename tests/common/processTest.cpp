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