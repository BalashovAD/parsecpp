#pragma once

#include <parsecpp/core/forward.h>

namespace prs {

namespace details {

template <typename T>
constexpr inline bool IsParserC = false;

template <typename T, typename Ctx, typename Fn>
constexpr inline bool IsParserC<Parser<T, Ctx, Fn>> = true;

}

template <typename T>
concept ParserType = details::IsParserC<std::decay_t<T>>;


template <ParserType Parser>
using GetParserResult = typename std::decay_t<Parser>::Type;


template <ParserType Parser>
using GetParserCtx = typename std::decay_t<Parser>::Ctx;

}