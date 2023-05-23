#include "forwardHeaderTest.cpp"
#include "../testHelper.h"

static inline auto p = charFrom('a').toCommonType();

ForwardTest::ForwardTest() noexcept
    : m_pimpl(p) {

}

bool ForwardTest::tryParse(std::string_view s) const {
    Stream stream(s);
    return !m_pimpl(stream).isError();
}


TEST(Pimple, Parser) {
    ForwardTest t;

    EXPECT_TRUE(t.tryParse("a"));
    EXPECT_TRUE(t.tryParse("ab"));
    EXPECT_FALSE(t.tryParse("b"));
}

TEST(Pimple, JustPimpl) {
    Pimpl<unsigned, debug::DebugContext> t((number<unsigned>() << debug::logPoint("a")).toCommonType());
    Stream s{"1"};
    debug::DebugContext ctx;
    auto result = t(s, ctx);
    ASSERT_FALSE(result.isError());
    EXPECT_EQ(result.data(), 1.f);
}