#include "../testHelper.h"

TEST(ApplyFirstMatch, Common) {
    auto applyFirstMatch = details::makeFirstMatch(0, std::make_tuple(1, 1), std::make_tuple(2, 2), std::make_tuple(3, 3));

    EXPECT_EQ(applyFirstMatch.apply(1), 1);
    EXPECT_EQ(applyFirstMatch.apply(2), 2);
    EXPECT_EQ(applyFirstMatch.apply(3), 3);
    EXPECT_EQ(applyFirstMatch.apply(4), 0);
}


TEST(ApplyFirstMatch, CommonConstexpr) {
    constexpr auto applyFirstMatch = details::makeFirstMatch(
            0, std::make_tuple(1, 1), std::make_tuple(2, 2), std::make_tuple(3, 3));

    static_assert(applyFirstMatch.apply(1) == 1);
    static_assert(applyFirstMatch.apply(2) == 2);
    static_assert(applyFirstMatch.apply(3) == 3);
    static_assert(applyFirstMatch.apply(999) == 0);
}


TEST(ApplyFirstMatch, EqualKeys) {
    auto applyFirstMatch = details::makeFirstMatch(
            0, std::make_tuple(1, 1), std::make_tuple(1, 2));

    EXPECT_EQ(applyFirstMatch.apply(1), 1);
}

TEST(ApplyFirstMatch, CustomEq) {
    using namespace std::string_view_literals;

    auto applyFirstMatch = details::makeFirstMatchEq(
            0, details::StartWith{}, std::make_tuple("ab"sv, 1), std::make_tuple("b"sv, 2), std::make_tuple("a"sv, 3));

    EXPECT_EQ(applyFirstMatch.apply("abtest"sv), 1);
    EXPECT_EQ(applyFirstMatch.apply("btest"sv), 2);
    EXPECT_EQ(applyFirstMatch.apply("atest"sv), 3);
    EXPECT_EQ(applyFirstMatch.apply("test"sv), 0);
}

struct TestCopyCtor {
    static inline unsigned cpy = 0;

    constexpr TestCopyCtor() = default;

    constexpr TestCopyCtor(TestCopyCtor&&) = default;

    TestCopyCtor(TestCopyCtor const& t) noexcept {
        ++cpy;
    }

    TestCopyCtor const& operator()(char c) const noexcept {
        static TestCopyCtor a;
        return a;
    }
};

TEST(ApplyFirstMatch, Copy) {
    using Q = TestCopyCtor;

    Q::cpy = 0;
    static constexpr auto firstMatch = details::makeFirstMatch(Q{}, std::make_tuple(5, Q{}));
    EXPECT_EQ(Q::cpy, 0); // call only move ctor

    Q::cpy = 0;
    [[maybe_unused]]
    auto a = firstMatch.apply(5);
    static_assert(std::is_same_v<decltype(a), TestCopyCtor>); // return const&, copy ctor in auto
    EXPECT_EQ(Q::cpy, 1);
    Q::cpy = 0;

    [[maybe_unused]]
    auto b = firstMatch.apply(3);
    static_assert(std::is_same_v<decltype(b), TestCopyCtor>);
    EXPECT_EQ(Q::cpy, 1);
    Q::cpy = 0;

    [[maybe_unused]]
    decltype(auto) c = firstMatch.apply(5, 'c');
    static_assert(std::is_same_v<decltype(c), TestCopyCtor const&>);
    EXPECT_EQ(Q::cpy, 0);
    Q::cpy = 0;

    [[maybe_unused]]
    decltype(auto) d = firstMatch.apply(2, 'c');
    static_assert(std::is_same_v<decltype(d), TestCopyCtor const&>);
    EXPECT_EQ(Q::cpy, 0);
    Q::cpy = 0;
}

TEST(ApplyFirstMatch, ReferenceConst) {
    int unhandled = 1;
    int first = 2;
    auto t = details::makeFirstMatch(std::cref(unhandled), std::make_tuple(1, std::cref(first)));
    unhandled = 3;
    first = 4;


    decltype(auto) ans = t.apply(1);
    static_assert(std::is_same_v<decltype(ans), int const&>);
    EXPECT_EQ(ans, 4);
    EXPECT_EQ(std::addressof(ans), std::addressof(first));

    decltype(auto) ans2 = t.apply(2);
    static_assert(std::is_same_v<decltype(ans2), int const&>);
    EXPECT_EQ(ans2, 3);
    EXPECT_EQ(std::addressof(ans2), std::addressof(unhandled));
}

TEST(ApplyFirstMatch, ReferenceNonConst) {
    int unhandled = 1;
    int first = 2;
    auto t = details::makeFirstMatch(std::ref(unhandled), std::make_tuple(1, std::ref(first)));
    unhandled = 3;
    first = 4;

    decltype(auto) ans = t.apply(1);
    static_assert(std::is_same_v<decltype(ans), int const&>);
    EXPECT_EQ(ans, 4);
    EXPECT_EQ(std::addressof(ans), std::addressof(first));

    decltype(auto) ans2 = t.apply(2);
    static_assert(std::is_same_v<decltype(ans2), int const&>);
    EXPECT_EQ(ans2, 3);
    EXPECT_EQ(std::addressof(ans2), std::addressof(unhandled));
}