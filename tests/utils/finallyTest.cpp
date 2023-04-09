#include "../testHelper.h"

TEST(Finally, Common) {
    int i = 0;
    {
        Finally changeValue([&i]() noexcept {
            ++i;
        });

        EXPECT_EQ(i, 0);
    }
    EXPECT_EQ(i, 1);
}


TEST(Finally, MoveCtor) {
    int i = 0;

    const auto f = [&i]() noexcept {
        ++i;
    };

    std::shared_ptr<Finally<decltype(f)>> spFinally;
    {
        Finally changeValue(f);
        EXPECT_EQ(i, 0);
        if (i == 0) {
            auto changeValue2{std::move(changeValue)};
            EXPECT_EQ(i, 0);
            i = 1;
            EXPECT_EQ(i, 1);
        }

        EXPECT_EQ(i, 2);
    }
    EXPECT_EQ(i, 2);
}


TEST(Finally, Release) {
    int i = 0;

    const auto f = [&i]() noexcept {
        ++i;
    };

    std::shared_ptr<Finally<decltype(f)>> spFinally;
    {
        Finally changeValue(f);
        EXPECT_EQ(i, 0);
        changeValue.release();
    }
    EXPECT_EQ(i, 0);
}