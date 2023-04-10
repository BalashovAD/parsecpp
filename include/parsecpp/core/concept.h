#pragma once

#include <parsecpp/core/context.h>

namespace prs {

template <typename T>
constexpr bool IsParser = false;

template <typename Fn, ContextType Ctx>
static constexpr bool IsParserFn = std::is_invocable_v<Fn, Stream&, Ctx&> || (IsVoidCtx<Ctx> && std::is_invocable_v<Fn, Stream&>);

template <typename T, ContextType Ctx, typename Fn>
    requires (IsParserFn<Fn, Ctx>)
class Parser;

template <typename T, ContextType Ctx, typename Fn>
constexpr bool IsParser<Parser<T, Ctx, Fn>> = true;


template <typename T>
concept ParserType = IsParser<T>;


template <ParserType Parser>
using ParserResult = typename std::decay_t<Parser>::Type;


template <ParserType Parser>
using ParserCtx = typename std::decay_t<Parser>::Ctx;

}