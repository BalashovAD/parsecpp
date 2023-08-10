#include "../testHelper.h"

TEST(ConstexprString, Common) {
    constexpr auto str1 = "test"_prs;
    constexpr auto str2 = "test2"_prs;

    static_assert(str1 != str2);
    EXPECT_EQ(str2.substr<4>().sv(), "test");
    EXPECT_EQ(str2.substr<4>().sv(), str1.sv());
    static_assert(str1 == str1);
    static_assert(str2.substr<4>() == str1);
    static_assert(str2.startsFrom(str1));
    EXPECT_EQ(str1.sv(), "test");
    EXPECT_STREQ(str1.c_str(), "test");
    EXPECT_EQ(str1.size(), 4);
}

TEST(ConstexprString, Modifiers) {
    constexpr auto str1 = "test"_prs;
    constexpr auto str2 = "test2"_prs;

    static_assert(str1 + "2"_prs == str2);
    static_assert(str1.substr<0>() == ""_prs);
    static_assert(str1.substr<100>() == str1);

    static_assert(str1.add('2') == str2);
    static_assert(str1.between('{') == "{"_prs + str1 + "{"_prs);
    static_assert(str1.between('{', '}') == "{"_prs + str1 + "}"_prs);
}
