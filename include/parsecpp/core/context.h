#pragma once

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
    using type = CtxType<T>;
};

template <typename T, typename K>
struct ConvertToTypeWrapperT<TypeWrapperB<T, K>> {
    using type = TypeWrapperB<T, std::decay_t<K>>;
};

template <typename T>
using ConvertToTypeWrapper = typename ConvertToTypeWrapperT<T>::type;


// Context wrapper
template <typename ...Ctx>
class ContextWrapperT;


template<>
class ContextWrapperT<> {
public:
    static constexpr bool iscontext = true;
    static constexpr size_t size = 0;
};


template <typename T, typename K>
class ContextWrapperT<TypeWrapperB<T, K>> {
public:
    using Type = T;
    using Key = K;
    using TypeWrapper = TypeWrapperB<T, K>;

    static constexpr bool iscontext = true;
    static constexpr size_t size = 1;

    explicit ContextWrapperT(T value = {}) noexcept
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

    static constexpr bool iscontext = true;
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

    static constexpr bool iscontext = true;
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

    static constexpr bool iscontext = true;
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
    static constexpr bool iscontext = true;
    static constexpr size_t size = sizeof...(Types);

    static_assert(size > 1);

    template <typename ...Args>
        requires(sizeof...(Args) == size)
    explicit ContextWrapperT(Args&& ...args) noexcept
        : ContextWrapperT<Types>(std::forward<Args>(args))... {

    }
};

// utils
template<typename T, typename = std::void_t<>>
struct IsCtxT : std::false_type {};

template<typename T>
struct IsCtxT<T, std::enable_if_t<T::iscontext, void>>
        : std::true_type {};


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
static inline VoidContext VOID_CONTEXT{};


template<typename Ctx>
constexpr inline bool IsCtx = details::IsCtxT<Ctx>::value;

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

template <ContextType Ctx1, ContextType ...Args>
struct unionImpl;

template <ContextType Ctx1>
struct unionImpl<Ctx1> {
    using Type = Ctx1;
};

template <ContextType Ctx1, ContextType Ctx2, ContextType ...Args>
struct unionImpl<Ctx1, Ctx2, Args...> {
    using Type = typename unionImpl<typename unionImpl<Ctx1>::Type, Args...>::Type;
};

template <ContextType Ctx1>
struct unionImpl<Ctx1, VoidContext> {
    using Type = Ctx1;
};

template <ContextType Ctx1, typename T>
    requires (containsTypeF<Ctx1, T>())
struct unionImpl<Ctx1, details::ContextWrapperT<T>> {
    using Type = Ctx1;
};

template <typename T1, typename T2>
    requires (!std::is_same_v<T1, T2>)
struct unionImpl<details::ContextWrapperT<T1>, details::ContextWrapperT<T2>> {
    using Type = ContextWrapper<T1, T2>;
};


template <typename ...Args, typename T>
    requires (!containsTypeF<ContextWrapperT<Args...>, T>())
struct unionImpl<ContextWrapperT<Args...>, details::ContextWrapperT<T>> {
    using Type = ContextWrapperT<Args..., T>;
};


template <typename Ctx1, typename Head, typename ...Args>
struct unionImpl<Ctx1, ContextWrapperT<Head, Args...>> {
    using Type = typename unionImpl<typename unionImpl<Ctx1, ContextWrapperT<Head>>::Type, ContextWrapperT<Args...>>::Type;
};


template <typename Ctx, typename T>
struct GetTypeWrapperT {
    using Type = std::tuple_element_t<details::findIndex<0, typename Ctx::TupleTypes, T>(), typename Ctx::TupleTypes>;
};

template <typename CtxType, typename T>
struct GetTypeWrapperT<ContextWrapperT<CtxType>, T> {
    using Type = typename CtxType::TypeWrapper;
};

template <typename Ctx, typename T>
    requires(containsTypeF<Ctx, T>())
using GetTypeWrapper = typename GetTypeWrapperT<Ctx, T>::Type;

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
