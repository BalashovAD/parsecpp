#pragma once

#include <parsecpp/core/parser.h>

#include <concepts>

namespace prs {

template <std::invocable Fn>
auto lazy(Fn genParser) noexcept {
    using ParserResult = parser_result_t<std::invoke_result_t<Fn>>;
    return Parser<ParserResult>::make([genParser](Stream& stream) {
        return genParser().apply(stream);
    });
}


template <typename tag, typename Fn>
struct LazyCached {
    using ParserResult = parser_result_t<std::invoke_result_t<Fn>>;

    explicit LazyCached(Fn generator) noexcept {
        if (!insideWork) {
            insideWork = true;
            cachedParser.emplace(generator());
        }
    }

    auto operator()(Stream& stream) const {
        return (*cachedParser)(stream);
    }

    inline static bool insideWork = false;
    inline static std::optional<std::invoke_result_t<Fn>> cachedParser;
};

template <typename tag, std::invocable Fn>
auto lazyCached(Fn genParser) noexcept {
    return make_parser(LazyCached<tag, Fn>{genParser});
}


}