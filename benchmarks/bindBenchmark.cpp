#include <benchmark/benchmark.h>

#include <parsecpp/full.hpp>

#include "benchmarkHelper.hpp"


using namespace prs;
using sv = std::string_view;

auto bindTag() {
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

template <template<typename,typename> typename Storage>
auto bindCacheTag() {
    auto tagParser = charFrom<'<'>() >> until<'>'>() << charFrom<'>'>();
    auto closeTagGen = makeTCacher<Storage, sv>([](sv tagName) {
        std::string closeTag;
        closeTag.reserve(2 + tagName.size() + 2);
        closeTag += "</";
        closeTag.append(tagName.data(), tagName.size());
        closeTag += '>';
        return concat(pure(tagName), until<'<'>() << literal(closeTag));
    });

    auto oneTag = tagParser.bind(closeTagGen);

    return (searchText<true>("<") >> oneTag).template repeat<30>();
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


auto cmpTagV2() {
    auto tag = charFrom<'<'>() >> until<'>'>() << charFrom<'>'>();
    auto closedTag = literal<"</"_prs>() >> until<'>'>() << charFrom<'>'>();
    static constexpr sv FAIL_TAG = "";
    auto oneTag = liftM([](sv lTag, sv text, sv rTag) {
        if (lTag != rTag) {
            return std::make_tuple(FAIL_TAG, text);
        } else {
            return std::make_tuple(lTag, text);
        }
    }, tag, until<'<'>(), closedTag).cond([](auto const& t) {
        return std::get<0>(t) != FAIL_TAG;
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

BENCHMARK_CAPTURE(BM_TagParser, Bind, bindTag(), "./tags.txt");
BENCHMARK_CAPTURE(BM_TagParser, BindMapCache, bindCacheTag<details::MapStorage>(), "./tags.txt");
BENCHMARK_CAPTURE(BM_TagParser, BindHashMapCache, bindCacheTag<details::HashMapStorage>(), "./tags.txt");
BENCHMARK_CAPTURE(BM_TagParser, BindVectorCache, bindCacheTag<details::VectorStorage>(), "./tags.txt");
BENCHMARK_CAPTURE(BM_TagParser, Cmp, cmpTag(), "./tags.txt");
BENCHMARK_CAPTURE(BM_TagParser, CmpV2, cmpTagV2(), "./tags.txt");
