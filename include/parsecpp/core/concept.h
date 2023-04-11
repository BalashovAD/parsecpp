#pragma once

#include <parsecpp/core/context.h>

namespace prs {

template <typename Fn, ContextType Ctx>
static constexpr bool IsParserFn = std::is_invocable_v<Fn, Stream&, Ctx&> || (IsVoidCtx<Ctx> && std::is_invocable_v<Fn, Stream&>);

template <typename T, ContextType Ctx, typename Fn>
requires (IsParserFn<Fn, Ctx>)
class Parser;

namespace details {

template <typename T>
constexpr inline bool IsParserC = false;

template <typename T, typename Ctx, typename Fn>
constexpr inline bool IsParserC<Parser<T, Ctx, Fn>> = true;

}

template <typename T>
concept ParserType = details::IsParserC<std::decay_t<T>>;


template <ParserType Parser>
using ParserResult = typename std::decay_t<Parser>::Type;


template <ParserType Parser>
using ParserCtx = typename std::decay_t<Parser>::Ctx;

}