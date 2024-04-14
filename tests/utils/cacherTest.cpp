#include "../testHelper.h"


TEST(Memoizer, CommonInt) {
    auto cacher = makeMapMemoizer<int>([](int a) {
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

    bool operator==(CtrCounter const& rhs) const {
        return rhs.cc == cc;
    }

    char cc;
};

TEST(Memoizer, CtrCounter) {
    auto cacher = makeTMemoizer<details::HashMapStorage, char>(details::MakeClass<CtrCounter>{});

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

TEST(Memoizer, MultiArguments) {
    int counter = 0;
    auto cacher = makeTMemoizer<details::VectorStorage, int, int>([&](int a, int b) {
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