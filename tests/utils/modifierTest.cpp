#include "../testHelper.h"

struct AddStringDesc {
    auto operator()(auto& parser, Stream& stream) const {
        auto start = stream.pos();
        return parser().map([&](auto&& t) {
            return std::make_pair(t, stream.get_sv(start, stream.pos()));
        });
    }
};

TEST(Modifier, Common) {
    auto parser = spaces() >> (number() * AddStringDesc{});

    success_parsing(parser, {1.2, "1.2"}, " 1.2y", "y");
    success_parsing(parser, {-1.2, "-1.2"}, "  -1.2y", "y");

    failed_parsing(parser, 1, " gggg");
}

TEST(Modifier, LeftCtx) {
    ContextWrapper<HitCounterType<>> ctx{0};
    auto parser = spaces() >> (hitCounter() >> number()) * AddStringDesc{};

    success_parsing(parser, {1.2, "1.2"}, " 1.2y", "y", ctx);
    success_parsing(parser, {-1.2, "-1.2"}, "  -1.2y", "y", ctx);

    failed_parsing(parser, 1, " gggg", ctx);
}


TEST(Modifier, LeftDoubleCtx) {
    ContextWrapper<HitCounterType<std::integral_constant<char, 'a'>>, HitCounterType<std::integral_constant<char, 'b'>>>
        ctx{0, 0};
    auto parser = spaces() >> (hitCounter<std::integral_constant<char, 'a'>>()
            >> hitCounter<std::integral_constant<char, 'b'>>() >> number()) * AddStringDesc{};

    success_parsing(parser, {1.2, "1.2"}, " 1.2y", "y", ctx);
    success_parsing(parser, {-1.2, "-1.2"}, "  -1.2y", "y", ctx);

    failed_parsing(parser, 1, " gggg", ctx);
}