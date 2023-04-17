#pragma once

// #include <parsecpp/core/forward.h>


// #include <parsecpp/core/context.h>


#include <concepts>

#include <cstddef>
#include <tuple>
#include <utility>

namespace prs {


namespace details {

// Type wrapper
template <typename T, typename K>
struct TypeWrapperB {
    using Type = T;
    using Key = K;
};


template <typename T>
using CtxType = TypeWrapperB<T, std::decay_t<T>>;

template <typename T, typename Name>
using NamedType = TypeWrapperB<T, Name>;


template <typename T>
struct ConvertToTypeWrapperT {
    using Type = CtxType<T>;
};

template <typename T, typename K>
struct ConvertToTypeWrapperT<TypeWrapperB<T, K>> {
    using Type = TypeWrapperB<T, std::decay_t<K>>;
};

template <typename T>
using ConvertToTypeWrapper = typename ConvertToTypeWrapperT<T>::Type;


// Context wrapper
template <typename ...Ctx>
class ContextWrapperT;


template<>
class ContextWrapperT<> {
public:
    static constexpr size_t size = 0;
};


template <typename T, typename K>
class ContextWrapperT<TypeWrapperB<T, K>> {
public:
    using Type = T;
    using Key = K;
    using TypeWrapper = TypeWrapperB<T, K>;

    static constexpr size_t size = 1;

    explicit ContextWrapperT(T value = T{}) noexcept
        : m_value(std::move(value)) {

    }

    Type& get() noexcept {
        return m_value;
    }

    Type const& get() const noexcept {
        return m_value;
    }
private:
    T m_value{};
};

template <typename T, typename K>
class ContextWrapperT<TypeWrapperB<T&, K>> {
public:
    using Type = T;
    using Key = K;
    using TypeWrapper = TypeWrapperB<T&, K>;

    static constexpr size_t size = 1;

    explicit ContextWrapperT(T& value) noexcept
        : m_value(value) {

    }

    Type& get() noexcept {
        return m_value;
    }

    Type const& get() const noexcept {
        return m_value;
    }
private:
    T& m_value;
};


template <typename T, typename K>
class ContextWrapperT<TypeWrapperB<T const, K>> {
public:
    using Type = T;
    using Key = K;
    using TypeWrapper = TypeWrapperB<T const, K>;

    static constexpr size_t size = 1;

    explicit ContextWrapperT(T const& value = T{}) noexcept
        : m_value(value) {

    }

    Type const& get() const noexcept {
        return m_value;
    }
private:
    T m_value{};
};


template <typename T, typename K>
class ContextWrapperT<TypeWrapperB<T const&, K>> {
public:
    using Type = T;
    using Key = K;
    using TypeWrapper = TypeWrapperB<T const&, K>;

    static constexpr size_t size = 1;

    explicit ContextWrapperT(T const& value) noexcept
        : m_value(value) {

    }

    Type const& get() const noexcept {
        return m_value;
    }
private:
    T const& m_value;
};


template <typename ...Types>
class ContextWrapperT : public ContextWrapperT<>, public ContextWrapperT<Types> ... {
public:
    using TupleTypes = std::tuple<Types...>;
    static constexpr size_t size = sizeof...(Types);

    static_assert(size > 1);

    template <typename ...Args>
        requires(sizeof...(Args) == size)
    explicit ContextWrapperT(Args&& ...args) noexcept
        : ContextWrapperT<Types>(std::forward<Args>(args))... {

    }

    explicit ContextWrapperT() noexcept = default;
};

// utils
template<typename T>
static constexpr bool IsCtx = false;

template <typename ...Args>
static constexpr bool IsCtx<ContextWrapperT<Args...>> = true;



template<size_t index, typename Tuple, typename T>
constexpr int findIndex() noexcept {
    if constexpr (index >= std::tuple_size_v<Tuple>) {
        return -1;
    } else if constexpr (std::is_same_v<typename std::tuple_element_t<index, Tuple>::Key, typename ConvertToTypeWrapper<T>::Key>) {
        return index;
    } else {
        return findIndex<index + 1, Tuple, T>();
    }
}

}

template <typename ...Args>
using ContextWrapper = details::ContextWrapperT<details::ConvertToTypeWrapper<Args>...>;

using VoidContext = ContextWrapper<>;

[[maybe_unused]]
static inline VoidContext VOID_CONTEXT{};


template<typename Ctx>
constexpr inline bool IsCtx = details::IsCtx<std::decay_t<Ctx>>;

template<typename Ctx>
concept ContextType = IsCtx<Ctx>;

template<ContextType Ctx>
constexpr inline bool IsVoidCtx = std::is_same_v<Ctx, VoidContext>;

namespace details {

template<ContextType Ctx>
constexpr size_t sizeCtx() noexcept {
    return Ctx::size;
}

template <ContextType Ctx, typename T>
constexpr bool containsTypeF() noexcept {
    constexpr auto size = sizeCtx<Ctx>();
    if constexpr (size == 0) {
        return false;
    } else if constexpr (size == 1) {
        return std::is_same_v<typename Ctx::Key, typename ConvertToTypeWrapper<T>::Key>;
    } else {
        return findIndex<0, typename Ctx::TupleTypes, T>() != -1;
    }
}


template <typename Ctx, typename T>
struct GetTypeWrapperT {
    using Type = std::tuple_element_t<details::findIndex<0, typename Ctx::TupleTypes, T>(), typename Ctx::TupleTypes>;
};

template <typename CtxType, typename T>
struct GetTypeWrapperT<ContextWrapperT<CtxType>, T> {
    using Type = CtxType;
};


template <typename Ctx, typename T>
        requires(containsTypeF<Ctx, T>())
using GetTypeWrapper = typename GetTypeWrapperT<Ctx, T>::Type;


template <ContextType Ctx1, ContextType ...Args>
struct unionImpl;

template <ContextType Ctx1>
struct unionImpl<Ctx1> {
    using Type = Ctx1;
};

template <ContextType Ctx1, ContextType Ctx2, ContextType ...Args>
struct unionImpl<Ctx1, Ctx2, Args...> {
    using Type = typename unionImpl<typename unionImpl<Ctx1, Ctx2>::Type, Args...>::Type;
};

template <ContextType Ctx1>
struct unionImpl<Ctx1, VoidContext> {
    using Type = Ctx1;
};

template <ContextType Ctx1, typename T>
    requires (containsTypeF<Ctx1, T>())
struct unionImpl<Ctx1, details::ContextWrapperT<T>> {
    static_assert(std::is_same_v<typename GetTypeWrapper<Ctx1, T>::Type, typename ConvertToTypeWrapper<T>::Type>,
            "Use the same key type for different value types. Use details::NamedType to split types.");
    using Type = Ctx1;
};

template <typename ...Args, typename T>
    requires (!containsTypeF<ContextWrapperT<Args...>, T>())
struct unionImpl<ContextWrapperT<Args...>, details::ContextWrapperT<T>> {
    using Type = ContextWrapperT<Args..., T>;
};


template <typename Ctx1, typename Head, typename ...Tail>
struct unionImpl<Ctx1, ContextWrapperT<Head, Tail...>> {
    using Type = typename unionImpl<typename unionImpl<Ctx1, ContextWrapperT<Head>>::Type, ContextWrapperT<Tail...>>::Type;
};

}

template <ContextType Ctx>
static constexpr size_t sizeCtx = details::sizeCtx<Ctx>();


template <ContextType Ctx, typename T>
static constexpr bool containsType = details::containsTypeF<Ctx, T>();


template <ContextType ...Args>
using UnionCtx = typename details::unionImpl<Args...>::Type;


template <typename T, ContextType Ctx>
    requires (containsType<Ctx, T>)
decltype(auto) get(Ctx& ctx) noexcept {
    if constexpr (Ctx::size == 1) {
        return ctx.get();
    } else {
        using TypeWrapper = details::GetTypeWrapper<Ctx, T>;
        return static_cast<ContextWrapper<TypeWrapper>>(ctx).get();
    }
}


}

// #include <parsecpp/core/parsingError.h>


// #include <parsecpp/core/buildParams.h>


#include <cstddef>

namespace prs {

#ifndef PRS_DISABLE_ERROR_LOG
static constexpr bool DISABLE_ERROR_LOG = false;
#define PRS_MAKE_ERROR(strError, pos) makeError(strError, pos);
#else
static constexpr bool DISABLE_ERROR_LOG = true;
#define PRS_MAKE_ERROR(strError, pos) makeError("", pos);
#endif

static constexpr size_t MAX_ITERATION = 1000000;

}

#include <string>

namespace prs::details {

struct ParsingError {
    std::string description;
    size_t pos{};
};


}

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