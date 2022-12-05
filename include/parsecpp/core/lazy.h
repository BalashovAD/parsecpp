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

}