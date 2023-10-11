#include <benchmark/benchmark.h>

#include <parsecpp/all.hpp>

#include "benchmarkHelper.hpp"


using namespace prs;

auto bindTag() {
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

    auto oneTag = tagParser.bind(closeTagGen);

    return (searchText<true>("<") >> oneTag).repeat<30>();;
}

auto cmpTag() {
    auto tag = charFrom<'<'>() >> until<'>'>() << charFrom<'>'>();
    auto closedTag = literal<"</"_prs>() >> until<'>'>() << charFrom<'>'>();
    auto oneTag = concat(tag, until<'<'>(), closedTag).cond([](auto const& t) {
        return std::get<0>(t) == std::get<2>(t);
    }).fmap([](auto &&t) {
        return std::make_tuple(std::get<0>(t), std::get<1>(t));
    });

    return (searchText<true>("<") >> oneTag).repeat<30>();
}


template <ParserType P>
void BM_TagParser(benchmark::State& state, P parser, std::string const& filename) {

    std::string test = readFile(filename);
    for (auto _ : state) {
        Stream s{test};
        auto result = parser(s);
        if (result.isError()) {
            state.SkipWithError("Cannot parse");
        } else if (result.data().size() != 30) {
            state.SkipWithError("Wrong answer");
        }
    }

    state.SetBytesProcessed(test.size() * state.iterations());
}

//BENCHMARK_CAPTURE(BM_TagParser, Bind, bindTag(), "./tags.txt");
//BENCHMARK_CAPTURE(BM_TagParser, Cmp, cmpTag(), "./tags.txt");
