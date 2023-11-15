#include "../testHelper.h"

TEST(Bind, Base) {
    auto parser = charFrom('A', 'B', 'C').bind([](char c) {
        return charFrom(c);
    });

    static_assert(std::is_same_v<GetParserResult<decltype(parser)>, char>);
    static_assert(IsVoidCtx<GetParserCtx<decltype(parser)>>);

    success_parsing(parser, 'A', "AA");
    success_parsing(parser, 'C', "CC");

    failed_parsing(parser, 1, "AB");
    failed_parsing(parser, 0, "DD");
}

TEST(Bind, SimpleTag) {
    using sv = std::string_view;

    auto tagParser = charFrom<'<'>() >> until<'>'>() << charFrom<'>'>();
    auto closeTagGen = [](sv tagName) {
        std::string closeTag;
        closeTag.reserve(2 + tagName.size() + 2);
        closeTag += "</";
        closeTag.append(tagName.data(), tagName.size());
        closeTag += '>';
        return concat(pure(tagName), until<'<'>() << literal(closeTag));
    };

    auto parser = spaces() >> tagParser.bind(closeTagGen);

    static_assert(std::is_same_v<GetParserResult<decltype(parser)>, std::tuple<sv, sv>>);
    static_assert(IsVoidCtx<GetParserCtx<decltype(parser)>>);

    success_parsing(parser, {"A", "text"}, "<A>text</A>");
    success_parsing(parser, {"TTT", ""}, "<TTT></TTT>a", "a");

    failed_parsing(parser, 7, "<A>test");
    failed_parsing(parser, 7, "<A>test</B>");
    failed_parsing(parser, 8, "<Atest</");
}