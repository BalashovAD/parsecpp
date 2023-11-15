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

TEST(Context, UnionSameTypes) {
    using CI = ContextWrapper<int>;
    using CU = UnionCtx<CI, CI>;
    static_assert(std::is_same_v<CU, CI>);

    // Should not build
//    using CIC = ContextWrapper<int const>;
//    using CU2 = UnionCtx<CI, CIC>;
//    static_assert(std::is_same_v<CU2, ContextWrapper<int>>);
//
//    using CICL = ContextWrapper<int const&>;
//    using CU3 = UnionCtx<CI, CICL>;
//    static_assert(std::is_same_v<CU3, ContextWrapper<int>>);
//
//    using CIVL = ContextWrapper<int volatile&>;
//    using CU4 = UnionCtx<CI, CIVL>;
//    static_assert(std::is_same_v<CU4, ContextWrapper<int>>);
//
//    using CU5 = UnionCtx<CIC, CICL>;
//    static_assert(std::is_same_v<CU5, ContextWrapper<int const>>);
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
    GetParserCtx<decltype(parser)> ctx{n, c};

    success_parsing(parser, 'b', "123bc", "c", ctx);
    EXPECT_EQ(n, 123);
    failed_parsing(parser, 3, "122abc", ctx);
    EXPECT_EQ(n, 122);
}


TEST(Context, MergeContext1) {
    auto parser = addContext<2>() >> charFrom('1') << addContext<1>();
    using P = decltype(parser);
    auto ctx = P::makeCtx();

    static_assert(std::is_same_v<P::Ctx,
            ContextWrapper<
                    details::NamedType<unsigned, uniqCtxName<0>>,
                    details::NamedType<unsigned, uniqCtxName<1>>,
                    details::NamedType<unsigned, uniqCtxName<2>>>>);

    success_parsing(parser, '1', "123", "23", ctx);
}

TEST(Context, MergeContext2) {
    auto parser = addContext<2>() >> charFrom('1') << addContext<1>();
    using P = decltype(parser);
    auto ctx = P::makeCtx();

    static_assert(std::is_same_v<P::Ctx,
            ContextWrapper<
                    details::NamedType<unsigned, uniqCtxName<0>>,
                    details::NamedType<unsigned, uniqCtxName<1>>,
                    details::NamedType<unsigned, uniqCtxName<2>>>>);

    success_parsing(parser, '1', "123", "23", ctx);
}


template <typename T>
struct SaveLastResult {
    using Ctx = ContextWrapper<details::NamedType<T&, SaveLastResult<T>>>;
    auto operator()(ModifyCallerI<T>& parser, Stream& s, Ctx& ctx) const {
        return parser().map([&ctx](T&& t) {
            get<SaveLastResult<T>>(ctx) = t;
            return t;
        });
    }
};


TEST(Context, ContextLink) {
    char c;
    auto parser = addContext<1>() >> charFrom('1') * ModifyWithContext<SaveLastResult<char>, SaveLastResult<char>::Ctx>() << addContext();
    using P = decltype(parser);
    auto ctx = parser.makeCtx(0, 0, c);

    static_assert(std::is_same_v<P::Ctx,
            ContextWrapper<
                    details::NamedType<unsigned, uniqCtxName<0>>,
                    details::NamedType<unsigned, uniqCtxName<1>>,
                    SaveLastResult<char>::Ctx::TypeWrapper>>);

    success_parsing(parser, '1', "123", "23", ctx);
    EXPECT_EQ(c, '1');
    success_parsing(parser, '1', "123", "23", ctx);
    EXPECT_EQ(c, '1');
}


TEST(Context, Pointer) {
    struct SaveChar {
        using Ctx = ContextWrapper<char*>;
        auto operator()(ModifyCallerI<char>& parser, Stream& s, Ctx& ctx) const {
            return parser().map([&ctx](char c) {
                *get<char*>(ctx) = c;
                return c;
            });
        }
    };

    char c;
    auto parser = charFrom(FromRange('0', '9')) * ModifyWithContext<SaveChar, SaveChar::Ctx>{};
    auto ctx = parser.makeCtx(&c);

    success_parsing(parser, '1', "123", "23", ctx);
    EXPECT_EQ(c, '1');
    success_parsing(parser, '2', "23", "3", ctx);
    EXPECT_EQ(c, '2');
}

template <int M>
struct Dummy {};


TEST(Context, sizeOf) {
    using Ctx1 = ContextWrapper<Dummy<0>>;
    static_assert(sizeof(Ctx1) == 1);

    using Ctx2 = ContextWrapper<Dummy<0>, Dummy<1>, Dummy<2>>;
    static_assert(sizeof(Ctx2) == 1);
    Ctx2 ctx;

    using Ctx3 = ContextWrapper<Dummy<0>, Dummy<3> const>;
    static_assert(sizeof(Ctx3) == 1);

    using Ctx4 = ContextWrapper<char>;
    static_assert(sizeof(Ctx4) == sizeof(char));
}
