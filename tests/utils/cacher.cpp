#include "../testHelper.h"


TEST(StaticCacher, CommonInt) {
    auto cacher = makeMapCacher<int>([](int a) {
        return a + 1;
    });

    EXPECT_EQ(cacher(1), 2);
    EXPECT_EQ(cacher(3), 4);
}

struct CtrCounter {
    static inline size_t i = 0;

    explicit CtrCounter(char c)
        : cc(c) {
        ++i;
    }

    bool operator==(CtrCounter rhs) {
        return rhs.cc == cc;
    }

    char cc;
};

TEST(StaticCacher, CtrCounter) {
    auto cacher = makeTCacher<details::HashMapStorage, char>(details::MakeClass<CtrCounter>{});

    CtrCounter::i = 0;
    EXPECT_EQ(CtrCounter::i, 0);
    EXPECT_EQ(cacher('1').cc, '1');
    EXPECT_EQ(CtrCounter::i, 1);
    EXPECT_EQ(cacher('2').cc, '2');
    EXPECT_EQ(CtrCounter::i, 2);
    EXPECT_EQ(cacher('1').cc, '1');
    EXPECT_EQ(CtrCounter::i, 2);
    CtrCounter::i = 0;
}

TEST(StaticCacher, MultiArguments) {
    int counter = 0;
    auto cacher = makeTCacher<details::VectorStorage, int, int>([&](int a, int b) {
        ++counter;
        return a + b;
    });

    EXPECT_EQ(counter, 0);
    EXPECT_EQ(cacher(1, 2), 3);
    EXPECT_EQ(counter, 1);
    EXPECT_EQ(cacher(1, 2), 3);
    EXPECT_EQ(counter, 1);
    EXPECT_EQ(cacher(3, 3), 6);
    EXPECT_EQ(counter, 2);
}