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