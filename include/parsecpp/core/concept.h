#pragma once

#include <parsecpp/core/forward.h>

namespace prs {

namespace details {

template <typename T>
constexpr inline bool IsParserC = false;

template <typename T, typename Ctx, typename Fn>
constexpr inline bool IsParserC<Parser<T, Ctx, Fn>> = true;

}

template <typename Parser>
concept ParserType = details::IsParserC<std::decay_t<Parser>>;


template <ParserType Parser>
using GetParserResult = typename std::decay_t<Parser>::Type;


template <ParserType Parser>
using GetParserCtx = typename std::decay_t<Parser>::Ctx;


template <typename T, typename Parser>
concept ParserTypeResult = std::is_same_v<GetParserResult<Parser>, T>;

template <typename T, typename Ctx, typename Parser>
concept ParserTypeSpec = std::is_same_v<GetParserResult<Parser>, T> && std::is_same_v<GetParserCtx<Parser>, Ctx>;


template<typename, typename = std::void_t<>>
struct HasTypeCtx : std::false_type { };

template<typename T>
struct HasTypeCtx<T, std::void_t<std::enable_if_t<IsCtx<typename T::Ctx>, void>>> : std::true_type {};

template <typename Modifier>
struct ContextTrait {
private:
    struct Dummy {
        using Ctx = VoidContext;
    };
public:
    using Ctx = typename std::conditional_t<HasTypeCtx<Modifier>::value,
            Modifier,
            Dummy>::Ctx;
};

template <typename T>
using GetContextTrait = typename ContextTrait<T>::Ctx;

}