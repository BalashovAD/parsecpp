#pragma once

#include <parsecpp/core/context.h>
#include <parsecpp/core/parsingError.h>

#include <functional>

namespace prs {

class Stream;
template <typename Fn, ContextType Ctx>
static constexpr bool IsParserFn = std::is_invocable_v<Fn, Stream&, Ctx&> || (IsVoidCtx<Ctx> && std::is_invocable_v<Fn, Stream&>);

template <typename T, ContextType Ctx, typename Fn>
requires (IsParserFn<Fn, Ctx>)
class Parser;


static constexpr size_t COMMON_PARSER_SIZE = sizeof(std::function<void(void)>);

template <typename L, typename R>
    requires(!std::same_as<std::decay_t<L>, std::decay_t<R>>)
class Expected;

template <typename T, typename Ctx = VoidContext>
class Pimpl {
public:
    Pimpl() = delete;

    template <typename ParserType>
    Pimpl(ParserType t) noexcept;

    ~Pimpl();

    Expected<T, details::ParsingError> operator()(Stream& s, Ctx& ctx) const;
private:
    alignas(std::function<void(void)>) std::byte m_storage[COMMON_PARSER_SIZE];
};


template <typename T>
class Pimpl<T, VoidContext> {
public:
    Pimpl() = delete;

    template <typename ParserType>
    Pimpl(ParserType t) noexcept;

    ~Pimpl();

    Expected<T, details::ParsingError> operator()(Stream& s) const;
private:
    alignas(std::function<void(void)>) std::byte m_storage[COMMON_PARSER_SIZE];
};

}