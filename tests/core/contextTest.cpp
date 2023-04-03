#include "../testHelper.h"

TEST(Context, Ctor) {
    using CI = ContextWrapper<int&>;
    using CS = ContextWrapper<std::string const&>;
    int i;
    std::string s;

    using CU = UnionCtx<CI, CS>;
    static_assert(std::is_same_v<CU, ContextWrapper<int&, std::string const&>>);
    CU t{i, s};

    auto& li = get<int>(t);
    auto const& ls = get<std::string>(t);

    EXPECT_EQ(&li, &i);
    EXPECT_EQ(&ls, &s);
}


TEST(Context, UnionTwo) {
    using CI = ContextWrapper<int>;
    using CS = ContextWrapper<std::string>;
    using CU = UnionCtx<CI, CS>;
    static_assert(std::is_same_v<CU, ContextWrapper<int, std::string>>);

    using CUU = UnionCtx<CU, CI>;
    static_assert(std::is_same_v<CUU, ContextWrapper<int, std::string>>);

    using CUUU = UnionCtx<CU, CS>;
    static_assert(std::is_same_v<CUUU, ContextWrapper<int, std::string>>);
}

TEST(Context, UnionOne) {
    using CI = ContextWrapper<int>;
    using CU = UnionCtx<CI, CI>;
    static_assert(std::is_same_v<CU, CI>);

    using CIC = ContextWrapper<int const>;
    using CU2 = UnionCtx<CI, CIC>;
    static_assert(std::is_same_v<CU2, ContextWrapper<int, int const>>);
}

TEST(Context, Parser) {
    using Ctx = ContextWrapper<char const>;
    const auto ctxParser = make_parser<Ctx>([](Stream& stream, Ctx& ctx) {
        return charFrom(get<char>(ctx)).apply(stream);
    });

    Ctx ctx{'b'};

    success_parsing(ctxParser, 'b', "ba", "a", ctx);

    char c = 'b';
    Ctx ctx2{c};
    success_parsing(ctxParser, 'b', "ba", "a", ctx2);

    failed_parsing(ctxParser, 0, "ab", ctx);
}

TEST(Context, OpForget) {
    using Ctx = ContextWrapper<char const>;
    const auto ctxParser = make_parser<Ctx>([](Stream& stream, Ctx& ctx) {
        return charFrom(get<char>(ctx)).apply(stream);
    });

    auto parser = charFrom('a') >> ctxParser;

    char const c = 'b';
    Ctx ctx{c};

    success_parsing(parser, 'b', "abc", "c", ctx);
    failed_parsing(parser, 0, "bac", ctx);
}

TEST(Context, OpForget2) {
    using CtxC = ContextWrapper<char const>;
    using CtxI = ContextWrapper<unsigned&>;
    const auto ctxParser = make_parser<CtxC>([](Stream& stream, CtxC& ctx) {
        return charFrom(get<char>(ctx)).apply(stream);
    });

    const auto ctxParserI = make_parser<CtxI>([](Stream& stream, CtxI& ctx) {
        return (number<unsigned>() >>= [&ctx](unsigned n) {
            get<unsigned>(ctx) = n;
            return n;
        }).apply(stream);
    });

    auto parser = ctxParserI >> ctxParser;

    char const c = 'b';
    unsigned n = 0;
    parser_ctx_t<decltype(parser)> ctx{n, c};

    success_parsing(parser, 'b', "123bc", "c", ctx);
    EXPECT_EQ(n, 123);
    failed_parsing(parser, 3, "122abc", ctx);
    EXPECT_EQ(n, 122);
}