#pragma once

// #include <parsecpp/core/forward.h>


// Needs for the correct single_include generation
#ifndef PARSECPP_FORWARD_H_GUARD
#define PARSECPP_FORWARD_H_GUARD

// #include <parsecpp/core/context.h>


// Needs for the correct single_include generation
#ifndef PARSECPP_CONTEXT_H_GUARD
#define PARSECPP_CONTEXT_H_GUARD

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

#endif

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
    explicit Pimpl(ParserType t) noexcept;

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
    explicit Pimpl(ParserType t) noexcept;

    ~Pimpl();

    Expected<T, details::ParsingError> operator()(Stream& s) const;
private:
    alignas(std::function<void(void)>) std::byte m_storage[COMMON_PARSER_SIZE];
};

}

#endif

// #include <parsecpp/core/parser.h>


// #include <parsecpp/core/expected.h>


// #include <parsecpp/utils/funcHelper.h>


#include <functional>
#include <memory>
#include <string>


namespace prs::details {

struct Id {
    template<typename T>
    auto operator()(T&& t) const noexcept {
        return std::forward<T>(t);
    }
};

struct ToString {
    template <typename T>
        requires (std::same_as<decltype(std::to_string(std::declval<T>())), std::string>)
    std::string operator()(T const& t) const noexcept {
        return std::to_string(t);
    }
};


struct MakeTuple {
    template <typename ...Args>
    auto operator()(Args &&...args) const noexcept {
        return std::make_tuple(std::forward<Args>(args)...);
    }
};


template <typename T>
struct MakeClass {
    template <class ...Args>
    decltype(auto) operator()(Args &&...args) const noexcept(std::is_nothrow_constructible_v<T, Args...>) {
        return T{std::forward<Args>(args)...};
    }
};

template <typename T>
struct MakeShared {
    template <class ...Args>
    auto operator()(Args &&...args) const {
        return std::make_shared<T>(std::forward<Args>(args)...);
    }
};


template <typename Fn>
class Y {
public:
    explicit Y(Fn fn) noexcept
        : m_fn(fn)
    {}

    template <typename ...Args>
    decltype(auto) operator()(Args&&...args) const {
        return m_fn(*this, std::forward<Args>(args)...);
    }

private:
    Fn m_fn;
};


template <size_t pw, typename T, typename Fn>
constexpr decltype(auto) repeatF(T t, Fn op) noexcept(std::is_nothrow_invocable_v<Fn, T>) {
    if constexpr (pw == 0) {
        return t;
    } else {
        return repeatF<pw - 1>(op(t), op);
    }
}

}


#include <utility>
#include <cassert>


namespace prs {

template <typename T, typename Error>
    requires(!std::same_as<std::decay_t<T>, std::decay_t<Error>>)
class Expected {
public:
    using Body = T;

    template<typename OnSuccess, typename OnError = details::Id>
    static constexpr bool map_nothrow = std::is_nothrow_invocable_v<OnSuccess, T const&>
                    && std::is_nothrow_invocable_v<OnError, Error const&>;

    template<typename OnSuccess, typename OnError = details::Id>
    static constexpr bool map_move_nothrow = std::is_nothrow_invocable_v<OnSuccess, T>
                    && std::is_nothrow_invocable_v<OnError, Error>;

    constexpr explicit Expected(T &&t) noexcept
        : m_isError(false)
        , m_data(std::move(t)) {

    }


    constexpr explicit Expected(T const& t) noexcept
        : m_isError(false)
        , m_data(t) {

    }

    constexpr explicit Expected(Error &&error) noexcept
        : m_isError(true)
        , m_error(std::move(error)) {
    }

    constexpr explicit Expected(Error const& error) noexcept
        : m_isError(true)
        , m_error(error) {
    }

    constexpr Expected(Expected<T, Error> const& e) noexcept
        : m_isError(e.m_isError) {

        if (isError()) {
            new (&m_error)Error(e.m_error);
        } else {
            new (&m_data)T(e.m_data);
        }
    }

    ~Expected() noexcept {
        if (isError()) {
            m_error.~Error();
        } else {
            m_data.~T();
        }
    }

    constexpr bool isError() const noexcept {
        return m_isError;
    }

    T const& data() const& noexcept {
        assert(!isError());
        return m_data;
    }

    T data() && noexcept {
        assert(!isError());
        return std::move(m_data);
    }

    Error const& error() const& noexcept {
        assert(isError());
        return m_error;
    }

    Error error() && noexcept {
        assert(isError());
        return std::move(m_error);
    }


    template<std::invocable<T const&> OnSuccess, std::invocable<Error const&> OnError>
    auto join(OnSuccess onSuccess, OnError onError) const&
                    noexcept(map_nothrow<OnSuccess, OnError>) {

//        using Result = std::common_type_t<std::invoke_result_t<OnSuccess, const T&>,
//                    std::invoke_result_t<OnError, const Error&>>;

        return isError() ? onError(m_error) : onSuccess(m_data);
    }


    template<std::invocable<T> OnSuccess, std::invocable<Error> OnError>
    auto join(OnSuccess onSuccess, OnError onError) &&
                    noexcept(map_nothrow<OnSuccess, OnError>) {

//        using Result = std::common_type_t<std::invoke_result_t<OnSuccess, const T&>,
//                    std::invoke_result_t<OnError, const Error&>>;

        return isError() ? onError(std::move(m_error)) : onSuccess(std::move(m_data));
    }

    template<std::invocable<const T&> OnSuccess>
    auto map(OnSuccess onSuccess) const&
                    noexcept(map_nothrow<OnSuccess>) {

        using Result = std::decay_t<std::invoke_result_t<OnSuccess, const T&>>;
        return isError() ? Expected<Result, Error>{m_error} : Expected<Result, Error>{onSuccess(m_data)};
    }


    template<std::invocable<T> OnSuccess>
    auto map(OnSuccess onSuccess) &&
                    noexcept(map_move_nothrow<OnSuccess>) {

        using Result = std::decay_t<std::invoke_result_t<OnSuccess, T&&>>;
        return isError() ? Expected<Result, Error>{std::move(m_error)}
                : Expected<Result, Error>{onSuccess(std::move(m_data))};
    }

    template<std::invocable<const T&> OnSuccess, std::invocable<Error const&> OnError>
    auto map(OnSuccess onSuccess, OnError onError) const& noexcept(map_nothrow<OnSuccess>) {

        using Result = std::invoke_result_t<OnSuccess, T const&>;
        return isError() ? Expected<Result, Error>{onError(m_error)}
                : Expected<Result, Error>{onSuccess(m_data)};
    }

    template<std::invocable<T> OnSuccess, std::invocable<Error> OnError>
    auto map(OnSuccess onSuccess, OnError onError) && noexcept(map_move_nothrow<OnSuccess>) {

        using Result = std::invoke_result_t<OnSuccess, T>;
        return isError() ? Expected<Result, Error>{onError(std::move(m_error))}
                : Expected<Result, Error>{onSuccess(std::move(m_data))};
    }

    template<std::invocable<T const&> OnSuccess>
    auto flatMap(OnSuccess onSuccess) const& noexcept(map_nothrow<OnSuccess>) {

        using Result = std::invoke_result_t<OnSuccess, T const&>;
        return isError() ? Result{m_error} : onSuccess(m_data);
    }

    template<std::invocable<T&&> OnSuccess>
    auto flatMap(OnSuccess onSuccess) && noexcept(map_nothrow<OnSuccess>) {

        using Result = std::invoke_result_t<OnSuccess, T>;
        return isError() ? Result{std::move(m_error)} : onSuccess(std::move(m_data));
    }


    template<std::invocable<T const&> OnSuccess, std::invocable<Error const&> OnError>
    auto flatMap(OnSuccess onSuccess, OnError onError) const& noexcept(map_nothrow<OnSuccess, OnError>) {

        return isError() ? onError(m_error) : onSuccess(m_data);
    }


    template<std::invocable<T const&> OnSuccess, std::invocable<Error const&> OnError>
    auto flatMap(OnSuccess onSuccess, OnError onError) && noexcept(map_nothrow<OnSuccess, OnError>) {

        return isError() ? onError(std::move(m_error)) : onSuccess(std::move(m_data));
    }

    template<std::invocable<Error const&> OnError>
    auto flatMapError(OnError onError) const& noexcept(map_nothrow<details::Id, OnError>) {
        using Result = std::invoke_result_t<OnError, Error const&>;
        return isError() ? onError(m_error) : Result{m_data};
    }

    template<std::invocable<Error> OnError>
    auto flatMapError(OnError onError) && noexcept(map_nothrow<details::Id, OnError>) {
        using Result = std::invoke_result_t<OnError, Error>;
        return isError() ? onError(std::move(m_error)) : Result{std::move(m_data)};
    }
private:
    bool m_isError;

    union {
        T m_data;
        Error m_error;
    };
};


}
// #include <parsecpp/utils/funcHelper.h>

// #include <parsecpp/utils/sourceLocation.h>


#include <string>
#include <sstream>


namespace prs::details {

// TODO: Replace to std::source_location
class SourceLocation
{
    static constexpr size_t hash(char const* str) noexcept {
        size_t hash = 5381;
        char c;
        while ((c = *str++) != '\0')
            hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

        return hash;
    }

    template <typename ...Args>
    static constexpr size_t combine(Args ...n) noexcept {
        size_t out = 0;
        ((out ^= (n + 0x9e3779b9 + (out << 6) + (out >> 2))), ...);
        return out;
    }
public:
    constexpr SourceLocation() noexcept
        : m_pFileName("unknown")
        , m_pFunctionName("unknown")
        , m_lineInFile(0) {
    }

    constexpr const char* fileNameBase() const noexcept {
        return BaseName(m_pFileName);
    }

    constexpr const char* fileNameFull() const noexcept {
        return m_pFileName;
    }

    constexpr const char* function() const noexcept {
        return m_pFunctionName;
    }

    constexpr int line() const noexcept {
        return m_lineInFile;
    }


    constexpr size_t hash() const noexcept {
        return combine(hash(m_pFileName), hash(m_pFunctionName), m_lineInFile);
    }

    static constexpr SourceLocation current(
            const char* pFileName = __builtin_FILE(),
            const char* pFunctionName = __builtin_FUNCTION(),
            int lineInFile = __builtin_LINE()) noexcept {
        SourceLocation sourceLocation;
        sourceLocation.m_pFileName = pFileName;
        sourceLocation.m_pFunctionName = pFunctionName;
        sourceLocation.m_lineInFile = lineInFile;
        return sourceLocation;
    }

    std::string prettyPrint() const noexcept {
        std::stringstream os;
        os << fileNameBase() << '(' << line() << ')';
        return os.str();
    }
private:
    static constexpr const char* stringEnd(const char* str) noexcept {
        return *str ? stringEnd(str + 1) : str;
    }

    static constexpr bool stringSlant(const char* str) noexcept {
        return *str == '/' || (*str != 0 && stringSlant(str + 1));
    }

    static constexpr const char* stringSlantRight(const char* str) noexcept {
        return *str == '/' ? (str + 1) : stringSlantRight(str - 1);
    }

    static constexpr const char* BaseName(const char* str) noexcept {
        return stringSlant(str) ? stringSlantRight(stringEnd(str)) : str;
    }

    const char* m_pFileName;
    const char* m_pFunctionName;
    size_t m_lineInFile;
};

}

// #include <parsecpp/core/parsingError.h>

// #include <parsecpp/core/baseTypes.h>


// #include <parsecpp/core/stream.h>


// #include <parsecpp/core/parsingError.h>

// #include <parsecpp/core/buildParams.h>


#include <string_view>
#include <string>
#include <utility>
#include <sstream>
#include <cassert>


namespace prs {

class Stream {
public:
    explicit Stream(std::string const& str) noexcept
        : Stream{str, str} {

    }

    explicit Stream(std::string_view str) noexcept
        : Stream{str, str} {
    }

    explicit Stream(char const* str) noexcept
        : Stream{std::string_view(str)} {
    }


    std::string_view& sv() noexcept {
        return m_currentStr;
    }

    char front() const noexcept {
        assert(!eos());
        return m_currentStr[0];
    }

    void move(size_t n = 1ull) noexcept {
        m_currentStr = m_currentStr.substr(std::min(n, m_currentStr.size()));
    }

    void moveUnsafe(size_t n = 1ull) noexcept {
        m_currentStr.remove_prefix(n);
    }

    std::string_view get_sv(size_t start, size_t end = std::string_view::npos) const noexcept(false) {
        return m_fullStr.substr(start, end - start);
    }

    std::string_view remaining() const noexcept {
        return m_currentStr;
    }

    std::string_view full() const noexcept {
        return m_fullStr;
    }

    size_t pos() const noexcept {
        return m_fullStr.size() - m_currentStr.size();
    }

    bool eos() const noexcept {
        return m_currentStr.empty();
    }

    template<std::predicate<char> Fn>
    char checkFirst(Fn const& test) noexcept(std::is_nothrow_invocable_v<Fn, char>) {
        if (eos()) {
            return 0;
        } else {
            char c = m_currentStr[0];
            if (test(c)) {
                m_currentStr.remove_prefix(1);
                return c;
            } else {
                return 0;
            }
        }
    }

    char checkFirst(char test) noexcept {
        if (eos()) {
            return 0;
        } else {
            char c = m_currentStr[0];
            if (test == c) {
                m_currentStr.remove_prefix(1);
                return c;
            } else {
                return 0;
            }
        }
    }

    void restorePos(size_t pos) noexcept {
        assert(pos <= m_fullStr.size());
        m_currentStr = m_fullStr.substr(pos);
    }

    template <unsigned printBefore = 3, unsigned printAfter = 3>
    std::string generateErrorText(details::ParsingError const& error) const noexcept {
        if (error.pos > m_fullStr.size()) {
            return "Wrong error pos";
        }

        std::ostringstream stream;
        stream << "Parse error in pos: " << error.pos;
        if constexpr (!DISABLE_ERROR_LOG) {
            stream << ", dsc: " << error.description;
        }
        if constexpr (printAfter + printBefore > 0) {
            auto startPos = error.pos > printBefore ? error.pos - printBefore : 0;
            auto endPos = error.pos + printAfter > m_fullStr.size() ? m_fullStr.size() : error.pos + printAfter;
            stream << ", text: |" << m_fullStr.substr(startPos, error.pos - startPos)
                    << "$" << m_fullStr.substr(error.pos, endPos - error.pos) << "|";
        }
        return stream.str();
    }
private:
    Stream(std::string_view cur, std::string_view full) noexcept
        : m_fullStr(cur)
        , m_currentStr(full) {

    }

    std::string_view const m_fullStr;
    std::string_view m_currentStr;
};


}
// #include <parsecpp/core/concept.h>


// #include <parsecpp/core/forward.h>


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
// #include <parsecpp/core/context.h>


namespace prs {


struct Unit {
    Unit() noexcept = default;

    bool operator==(Unit const&) const noexcept {
        return true;
    }
};


struct Drop {
    Drop() noexcept = default;

    bool operator==(Drop const&) const noexcept {
        return true;
    }
};

namespace details {

template <class Body>
using ResultType = Expected<Body, ParsingError>;

template <typename T, ContextType Ctx = VoidContext>
using StdFunction = std::conditional_t<IsVoidCtx<Ctx>,
                std::function<ResultType<T>(Stream&)>, std::function<ResultType<T>(Stream&, Ctx&)>>;

}



};

// #include <parsecpp/core/concept.h>

// #include <parsecpp/core/context.h>

// #include <parsecpp/core/stream.h>


#include <concepts>
#include <string_view>
#include <functional>
#include <utility>
#include <optional>


namespace prs {

template <typename T, ContextType CtxType = VoidContext, typename Func = details::StdFunction<T, CtxType>>
        requires (IsParserFn<Func, CtxType>)
class Parser {
public:
    static constexpr bool nocontext = IsVoidCtx<CtxType>;
    static constexpr bool nothrow = std::conditional_t<nocontext && std::is_invocable_v<Func, Stream&>,
                    std::is_nothrow_invocable<Func, Stream&>,
                    std::is_nothrow_invocable<Func, Stream&, CtxType&>>::value;

    using StoredFn = std::decay_t<Func>;

    using Type = T;
    using Ctx = CtxType;
    using Result = details::ResultType<T>;


    template <typename Fn>
        requires (IsParserFn<Func, Ctx>)
    constexpr static auto make(Fn &&f) noexcept {
        return Parser<T, Ctx, Fn>(std::forward<Fn>(f));
    }


    template <typename U>
        requires(std::is_same_v<std::decay_t<U>, StoredFn>)
    constexpr explicit Parser(U&& f) noexcept
        : m_fn(std::forward<U>(f)) {

    }

    constexpr Result operator()(Stream& stream) const noexcept(nothrow) requires(nocontext) {
        if constexpr (!std::is_invocable_v<StoredFn, Stream&> && nocontext) {
            return std::invoke(m_fn, stream, VOID_CONTEXT);
        } else {
            return std::invoke(m_fn, stream);
        }
    }

    template <ContextType Context>
    constexpr Result operator()(Stream& stream, Context& ctx) const noexcept(nothrow) {
        if constexpr (nocontext && std::is_invocable_v<StoredFn, Stream&>) {
            return std::invoke(m_fn, stream);
        } else {
            return std::invoke(m_fn, stream, ctx);
        }
    }


    constexpr Result apply(Stream& stream) const noexcept(nothrow) requires(nocontext) {
        return operator()(stream);
    }

    template <typename Context>
    constexpr Result apply(Stream& stream, Context& ctx) const noexcept(nothrow) {
        return operator()(stream, ctx);
    }


    template <typename ...Args>
        requires(std::is_constructible_v<Ctx, Args...>)
    static Ctx makeCtx(Args&& ...args) noexcept(std::is_nothrow_constructible_v<Ctx, Args...>) {
        return Ctx{std::forward<Args>(args)...};
    }

    /**
     * @def `>>` :: Parser<A> -> Parser<B> -> Parser<B>
     */
    template <typename B, typename CtxB, typename Rhs>
        requires (IsVoidCtx<UnionCtx<Ctx, CtxB>>)
    constexpr auto operator>>(Parser<B, CtxB, Rhs> rhs) const noexcept {
        return Parser<B, VoidContext>::make([lhs = *this, rhs](Stream& stream) noexcept(nothrow && Parser<B, CtxB, Rhs>::nothrow) {
            return lhs.apply(stream).flatMap([&rhs, &stream](T const& body) noexcept(Parser<B, CtxB, Rhs>::nothrow) {
                return rhs.apply(stream);
            });
        });
    }


    /**
     * @def `>>` :: Parser<A, CtxA> -> Parser<B, CtxB> -> Parser<B, CtxA & CtxB>
     */
    template <typename B, typename CtxB, typename Rhs>
        requires (!IsVoidCtx<UnionCtx<Ctx, CtxB>>)
    constexpr auto operator>>(Parser<B, CtxB, Rhs> rhs) const noexcept {
        return Parser<B, UnionCtx<Ctx, CtxB>>::make([lhs = *this, rhs](Stream& stream, auto& ctx) noexcept(nothrow && Parser<B, CtxB, Rhs>::nothrow) {
            return lhs.apply(stream, ctx).flatMap([&rhs, &stream, &ctx](T const& body) noexcept(Parser<B, CtxB, Rhs>::nothrow) {
                return rhs.apply(stream, ctx);
            });
        });
    }


    /**
     * @def `<<` :: Parser<A> -> Parser<B> -> Parser<A>
     */
    template <typename B, typename CtxB, typename Rhs>
        requires (IsVoidCtx<UnionCtx<Ctx, CtxB>>)
    constexpr auto operator<<(Parser<B, CtxB, Rhs> rhs) const noexcept {
        constexpr bool firstCallNoexcept = nothrow && Parser<B, CtxB, Rhs>::nothrow;
        return Parser<T>::make([lhs = *this, rhs](Stream& stream) noexcept(firstCallNoexcept) {
            return lhs.apply(stream).flatMap([&rhs, &stream](T&& body) noexcept(Parser<B, CtxB, Rhs>::nothrow) {
                return rhs.apply(stream).map([mvBody = std::move(body)](auto const& _) {
                    return mvBody;
                });
            });
        });
    }


    /**
     * @def `<<` :: Parser<A, CtxA> -> Parser<B, CtxB> -> Parser<A, CtxA & CtxB>
     */
    template <typename B, typename CtxB, typename Rhs>
        requires (!IsVoidCtx<UnionCtx<Ctx, CtxB>>)
    constexpr auto operator<<(Parser<B, CtxB, Rhs> rhs) const noexcept {
        constexpr bool firstCallNoexcept = nothrow && Parser<B, CtxB, Rhs>::nothrow;
        return Parser<T, UnionCtx<Ctx, CtxB>>::make([lhs = *this, rhs](Stream& stream, auto& ctx) noexcept(firstCallNoexcept) {
            return lhs.apply(stream, ctx).flatMap([&rhs, &stream, &ctx](T body) noexcept(Parser<B, CtxB, Rhs>::nothrow) {
                return rhs.apply(stream, ctx).map([mvBody = std::move(body)](auto const& _) {
                    return mvBody;
                });
            });
        });
    }


    /*
     * <$>, fmap operator
     * @def `>>=` :: Parser<A> -> (A -> B) -> Parser<B>
     */
    template <typename ListFn>
        requires(nocontext)
    constexpr friend auto operator>>=(Parser lhs, ListFn fn) noexcept {
        using ResultT = std::decay_t<std::invoke_result_t<ListFn, T>>;
        return Parser<ResultT, Ctx>::make([lhs, fn](Stream& stream) {
           return lhs.apply(stream).map(fn);
        });
    }


    /*
     * <$>, fmap operator
     * @def `>>=` :: Parser<A, Ctx> -> (A -> B) -> Parser<B, Ctx>
     */
    template <std::invocable<T> ListFn>
        requires(!nocontext)
    constexpr friend auto operator>>=(Parser lhs, ListFn fn) noexcept {
        using ResultT = std::decay_t<std::invoke_result_t<ListFn, T>>;
        return Parser<ResultT, Ctx>::make([lhs, fn](Stream& stream, auto& ctx) {
           return lhs.apply(stream, ctx).map(fn);
        });
    }


    /**
     * @def `|` :: Parser<A> -> Parser<A> -> Parser<A>
     */
    template <typename CtxB, typename Rhs>
        requires (IsVoidCtx<UnionCtx<Ctx, CtxB>>)
    constexpr auto operator|(Parser<T, CtxB, Rhs> rhs) const noexcept {
        return Parser<T>::make([lhs = *this, rhs](Stream& stream) {
            auto backup = stream.pos();
            return lhs.apply(stream).flatMapError([&](details::ParsingError const& firstError) {
                stream.restorePos(backup);
                return rhs.apply(stream).flatMapError([&](details::ParsingError const& secondError) {
                    return PRS_MAKE_ERROR(
                            "Or error: " + firstError.description + ", " + secondError.description
                                 , std::max(firstError.pos, secondError.pos));
                });
            });
        });
    }

    /**
     * @def `|` :: Parser<A, CtxA> -> Parser<A, CtxB> -> Parser<A, CtxA & CtxB>
     */
    template <typename CtxB, typename Rhs>
        requires (!IsVoidCtx<UnionCtx<Ctx, CtxB>>)
    constexpr auto operator|(Parser<T, CtxB, Rhs> rhs) const noexcept {
        return Parser<T, UnionCtx<Ctx, CtxB>>::make([lhs = *this, rhs](Stream& stream, auto& ctx) {
            auto backup = stream.pos();
            return lhs.apply(stream, ctx).flatMapError([&](details::ParsingError const& firstError) {
                stream.restorePos(backup);
                return rhs.apply(stream, ctx).flatMapError([&](details::ParsingError const& secondError) {
                    return PRS_MAKE_ERROR(
                            "Or error: " + firstError.description + ", " + secondError.description
                                 , std::max(firstError.pos, secondError.pos));
                });
            });
        });
    }


    template <typename A>
    using MaybeValue = std::conditional_t<std::is_same_v<A, Drop>, Drop, std::optional<A>>;

    /**
     * @def maybe :: Parser<A> -> Parser<std::optional<B>>
     * maybe :: Parser<Drop> -> Parser<Drop>
     */
    constexpr auto maybe() const noexcept {
        if constexpr (nocontext) {
            return Parser<MaybeValue<T>>::make([parser = *this](Stream& stream) {
                auto backup = stream.pos();
                return parser.apply(stream).map([](T t) {
                    if constexpr (std::is_same_v<T, Drop>) {
                        return Drop{};
                    } else {
                        return MaybeValue<T>{std::move(t)};
                    }
                }, [&stream, &backup](details::ParsingError const& error) {
                    stream.restorePos(backup);
                    return MaybeValue<T>{};
                });
            });
        } else {
            return Parser<MaybeValue<T>, Ctx>::make([parser = *this](Stream& stream, auto& ctx) {
                auto backup = stream.pos();
                return parser.apply(stream, ctx).map([](T t) {
                    if constexpr (std::is_same_v<T, Drop>) {
                        return Drop{};
                    } else {
                        return MaybeValue<T>{std::move(t)};
                    }
                }, [&stream, &backup](details::ParsingError const& error) {
                    stream.restorePos(backup);
                    return MaybeValue<T>{};
                });
            });
        }
    }


    /**
     * @def maybeOr :: Parser<A> -> Parser<A>
     */
    constexpr auto maybeOr(T const& defaultValue) const noexcept {
        if constexpr (nocontext) {
            return Parser<T>::make([parser = *this, defaultValue](Stream& stream) {
                auto backup = stream.pos();
                return parser.apply(stream).flatMapError([&stream, &backup, &defaultValue](details::ParsingError const& error) {
                    stream.restorePos(backup);
                    return data(defaultValue);
                });
            });
        } else {
            return Parser<T, Ctx>::make([parser = *this, defaultValue](Stream& stream, auto& ctx) {
                auto backup = stream.pos();
                return parser.apply(stream, ctx).flatMapError([&stream, &backup, &defaultValue](details::ParsingError const& error) {
                    stream.restorePos(backup);
                    return data(defaultValue);
                });
            });
        }
    }


    /**
     * @def repeat :: Parser<A> -> Parser<std::vector<A>>
     */
    template <size_t reserve = 0, size_t maxIteration = MAX_ITERATION>
            requires(!std::is_same_v<T, Drop>)
    constexpr auto repeat() const noexcept {
        using Value = T;
        using P = Parser<std::vector<Value>, Ctx>;
        return P::make([value = *this](Stream& stream, auto& ctx) {
            std::vector<Value> out{};
            out.reserve(reserve);

            size_t iteration = 0;

            auto backup = stream.pos();
            do {
                backup = stream.pos();
                auto result = value.apply(stream, ctx);
                if (!result.isError()) {
                    out.emplace_back(std::move(result).data());
                } else {
                    stream.restorePos(backup);
                    return P::data(std::move(out));
                }
            } while (++iteration != maxIteration);

            return P::makeError("Max iteration", stream.pos());
        });
    }

    /**
     * @def repeat :: Parser<Drop> -> Parser<Drop>
     */
    template <size_t maxIteration = MAX_ITERATION>
            requires(std::is_same_v<T, Drop>)
    constexpr auto repeat() const noexcept {
        using P = Parser<Drop, Ctx>;
        return P::make([value = *this](Stream& stream, auto& ctx) noexcept(nothrow) {
            size_t iteration = 0;

            auto backup = stream.pos();
            do {
                backup = stream.pos();
                auto result = value.apply(stream, ctx);
                if (result.isError()) {
                    stream.restorePos(backup);
                    return P::data({});
                }
            } while (++iteration != maxIteration);

            return P::makeError("Max iteration", stream.pos());
        });
    }


    /**
     * @def repeat :: Parser<A> -> Parser<Delim> -> Parser<std::vector<A>>
     */
    template <size_t reserve = 0, size_t maxIteration = MAX_ITERATION, ParserType Delimiter>
            requires(!std::is_same_v<T, Drop>)
    constexpr auto repeat(Delimiter tDelimiter) const noexcept {
        using Value = T;
        using UCtx = UnionCtx<Ctx, GetParserCtx<Delimiter>>;
        using P = Parser<std::vector<Value>, UCtx>;
        constexpr bool noexceptP = nothrow && Delimiter::nothrow;
        return P::make([value = *this, delimiter = std::move(tDelimiter)](Stream& stream, auto& ctx) noexcept(noexceptP) {
            std::vector<Value> out{};
            out.reserve(reserve);

            size_t iteration = 0;

            auto backup = stream.pos();
            do {
                auto result = value.apply(stream, ctx);
                if (!result.isError()) {
                    out.emplace_back(std::move(result).data());
                } else {
                    return P::data(std::move(out));
                }

                backup = stream.pos();
            } while (!delimiter.apply(stream, ctx).isError() && ++iteration != maxIteration);

            if (iteration == maxIteration) {
                return P::makeError("Max iteration", stream.pos());
            } else {
                stream.restorePos(backup);
                return P::data(std::move(out));
            }
        });
    }


    /**
     * @def repeat :: Parser<Drop> -> Parser<Delim> -> Parser<Drop>
     */
    template <size_t maxIteration = MAX_ITERATION, ParserType Delimiter>
            requires(std::is_same_v<T, Drop>)
    constexpr auto repeat(Delimiter tDelimiter) const noexcept {
        using UCtx = UnionCtx<Ctx, GetParserCtx<Delimiter>>;
        using P = Parser<Drop, UCtx>;
        return P::make([value = *this, delimiter = std::move(tDelimiter)](Stream& stream, auto& ctx) {
            size_t iteration = 0;

            auto backup = stream.pos();
            do {
                auto result = value.apply(stream, ctx);
                if (result.isError()) {
                    return P::data(Drop{});
                }

                backup = stream.pos();
            } while (!delimiter.apply(stream, ctx).isError() && ++iteration != maxIteration);

            if (iteration == maxIteration) {
                return P::makeError("Max iteration", stream.pos());
            } else {
                stream.restorePos(backup);
                return P::data(Drop{});
            }
        });
    }


    /**
     * @def cond :: Parser<A> -> (A -> bool) -> Parser<A>
     */
    template <std::predicate<T const&> Fn>
            requires (!std::predicate<T const&, Stream&> && nocontext)
    constexpr auto cond(Fn test) const noexcept {
        return Parser<T>::make([parser = *this, test](Stream& stream) {
           return parser.apply(stream).flatMap([&test, &stream](T t) {
               if (test(t)) {
                   return Parser<T>::data(std::move(t));
               } else {
                   return Parser<T>::makeError("Cond failed", stream.pos());
               }
           });
        });
    }

    /**
     * @def cond :: Parser<A> -> (A -> bool) -> Parser<A>
     */
    template <std::predicate<T const&> Fn>
            requires (!std::predicate<T const&, Stream&> && !nocontext)
    constexpr auto cond(Fn test) const noexcept {
        return Parser<T, Ctx>::make([parser = *this, test](Stream& stream, auto& ctx) {
           return parser.apply(stream, ctx).flatMap([&test, &stream](T t) {
               if (test(t)) {
                   return Parser<T>::data(std::move(t));
               } else {
                   return Parser<T>::makeError("Cond failed", stream.pos());
               }
           });
        });
    }

    /**
     * @def cond :: Parser<A> -> (A -> Stream& -> bool) -> Parser<A>
     */
    template <std::predicate<T const&, Stream&> Fn>
        requires(nocontext)
    constexpr auto cond(Fn test) const noexcept {
        return Parser<T>::make([parser = *this, test](Stream& stream) {
           return parser.apply(stream).flatMap([&test, &stream](T t) {
               if (test(t, stream)) {
                   return Parser<T>::data(std::move(t));
               } else {
                   return Parser<T>::makeError("Cond failed", stream.pos());
               }
           });
        });
    }

    /**
     * @def cond :: Parser<A> -> (A -> Stream& -> bool) -> Parser<A>
     */
    template <std::predicate<T const&, Stream&> Fn>
        requires(!nocontext)
    constexpr auto cond(Fn test) const noexcept {
        return Parser<T, Ctx>::make([parser = *this, test](Stream& stream, auto& ctx) {
           return parser.apply(stream, ctx).flatMap([&test, &stream](T t) {
               if (test(t, stream)) {
                   return Parser<T>::data(std::move(t));
               } else {
                   return Parser<T>::makeError("Cond failed", stream.pos());
               }
           });
        });
    }


    /**
     * @def cond :: Parser<A, Ctx> -> (A -> CondCtx& -> bool) -> Parser<A, Ctx & CondCtx>
     */
    template <ContextType CondContext, std::predicate<T const&, CondContext&> Fn>
    constexpr auto condC(Fn test) const noexcept {
        using UCtx = UnionCtx<Ctx, CondContext>;
        return Parser<T, UCtx>::make([parser = *this, test](Stream& stream, auto& ctx) {
           return parser.apply(stream, ctx).flatMap([&](T t) {
               if (test(t, ctx)) {
                   return Parser<T>::data(std::move(t));
               } else {
                   return Parser<T>::makeError("Cond failed", stream.pos());
               }
           });
        });
    }

    /**
     * @def Drop :: Parser<A> -> Parser<Drop>
     */
    constexpr auto drop() const noexcept {
        if constexpr (nocontext) {
            return Parser<Drop>::make([p = *this](Stream& s) {
                return p.apply(s).map([](auto &&) {
                    return Drop{};
                });
            });
        } else {
            return Parser<Drop, Ctx>::make([p = *this](Stream& s, auto& ctx) {
                return p.apply(s, ctx).map([](auto &&) {
                    return Drop{};
                });
            });
        }
    }

    /**
     * @def endOfStream :: Parser<A> -> Parser<A>
     */
    constexpr auto endOfStream() const noexcept {
        return cond([](T const& t, Stream& s) {
            return s.eos();
        });
    }

    /**
     * @def mustConsume :: Parser<A> -> Parser<A>
     */
    constexpr auto mustConsume() const noexcept {
        return make([p = *this](Stream& s, auto& ctx) {
            auto pos = s.pos();
            return p.apply(s, ctx).flatMap([pos, &s](auto &&t) {
               if (s.pos() > pos) {
                   return data(t);
               } else {
                   return makeError("Didn't consume stream", s.pos());
               }
            });
        });
    }


    template <typename Fn>
        requires(nocontext)
    constexpr auto flatMap(Fn fn) const noexcept(std::is_nothrow_invocable_v<Fn, T const&>) {
        return make_parser([parser = *this, fn](Stream& stream) {
            return parser.apply(stream).flatMap([fn, &stream](T const& t) {
                return fn(t).apply(stream);
            });
        });
    }


    template <typename Fn>
        requires(!nocontext)
    constexpr auto flatMap(Fn fn) const noexcept(std::is_nothrow_invocable_v<Fn, T const&>) {
        return make_parser<Ctx>([parser = *this, fn](Stream& stream, auto& ctx) {
            return parser.apply(stream, ctx).flatMap([fn, &stream](T const& t) {
                return fn(t).apply(stream);
            });
        });
    }

    /**
     * @def toCommonType :: Parser<A, Ctx, Fn> -> Parser'<A, Ctx>
     */
    constexpr auto toCommonType() const noexcept {
        if constexpr (nocontext) {
            details::StdFunction<T> f = [fn = *this](Stream& stream) {
                return std::invoke(fn, stream);
            };

            return Parser<T>(f);
        } else {
            details::StdFunction<T, Ctx> f = [fn = *this](Stream& stream, auto& ctx) {
                return std::invoke(fn, stream, ctx);
            };

            return Parser<T, Ctx>(f);
        }
    }

    static Result makeError(details::ParsingError error) noexcept {
        return Result{error};
    }

    static constexpr Result makeError(size_t pos) noexcept {
        return Result{details::ParsingError{"", pos}};
    }

#ifdef PRS_DISABLE_ERROR_LOG
    static constexpr Result makeError(std::string_view desc, size_t pos) noexcept {
        return Result{details::ParsingError{"", pos}};
    }
#else
    static constexpr Result makeError(
            std::string desc,
            size_t pos,
            details::SourceLocation source = details::SourceLocation::current()) noexcept {
        return Result{details::ParsingError{std::move(desc), pos}};
    }
#endif

    template <typename U = T>
        requires(std::convertible_to<U, T>)
    static constexpr Result data(U&& t) noexcept(std::is_nothrow_convertible_v<U, T>) {
        return Result{std::forward<U>(t)};
    }
private:
    StoredFn m_fn;
};

}


// #include <parsecpp/core/forwardImpl.h>


// #include <parsecpp/core/parser.h>

// #include <parsecpp/core/forward.h>


namespace prs {


template <typename T, typename Ctx>
template <typename ParserType>
Pimpl<T, Ctx>::Pimpl(ParserType t) noexcept {
    static_assert(std::is_same_v<std::decay_t<ParserType>, Parser<T, Ctx>>, "Pimpl can store only Common parser type");
    static_assert(sizeof(t) == sizeof(m_storage));
    static_assert(alignof(ParserType) == alignof(std::function<void(void)>));
    new (&m_storage) ParserType(t);
}


template <typename T>
template <typename ParserType>
Pimpl<T, VoidContext>::Pimpl(ParserType t) noexcept {
    static_assert(std::is_same_v<std::decay_t<ParserType>, Parser<T>>, "Pimpl can store only Common parser type");
    static_assert(sizeof(t) == sizeof(m_storage));
    static_assert(alignof(ParserType) == alignof(std::function<void(void)>));
    new (&m_storage)ParserType(t);
}


template <typename T, typename Ctx>
Pimpl<T, Ctx>::~Pimpl() {
    using P = Parser<T, Ctx>;
    std::destroy_at(std::launder(reinterpret_cast<P*>(&m_storage)));
}


template <typename T>
Pimpl<T>::~Pimpl() {
    using P = Parser<T>;
    std::destroy_at(std::launder(reinterpret_cast<P*>(&m_storage)));
}


template<typename T, typename Ctx>
Expected<T, details::ParsingError> Pimpl<T, Ctx>::operator()(Stream& s, Ctx& ctx) const {
    using P = Parser<T, Ctx>;
    return reinterpret_cast<P const&>(m_storage).apply(s, ctx);
}


template<typename T>
Expected<T, details::ParsingError> Pimpl<T, VoidContext>::operator()(Stream& s) const {
    using P = Parser<T>;
    return reinterpret_cast<P const&>(m_storage).apply(s);
}

}
// #include <parsecpp/core/lazy.h>


// #include <parsecpp/core/parser.h>


#include <source_location>

namespace prs {

template <std::invocable Fn>
auto lazy(Fn genParser) noexcept {
    using P = std::invoke_result_t<Fn>;
    if constexpr (P::nocontext) {
        return P::make([genParser](Stream& stream) {
            return genParser().apply(stream);
        });
    } else {
        return P::make([genParser](Stream& stream, auto& ctx) {
            return genParser().apply(stream, ctx);
        });
    }
}


template <typename tag, typename Fn>
class LazyCached {
public:
    using P = std::invoke_result_t<Fn>;
    using ParserResult = GetParserResult<P>;
    static constexpr bool nocontext = IsVoidCtx<GetParserCtx<P>>;

    explicit LazyCached(Fn const& generator) noexcept {
        if (!insideWork) { // cannot use optional.has_value() because generator() invoke ctor before optional::emplace
            insideWork = true;
            cachedParser.emplace(generator());
        }
    }

    auto operator()(Stream& stream) const {
        return (*cachedParser)(stream);
    }

    auto operator()(Stream& stream, auto& ctx) const {
        return (*cachedParser)(stream, ctx);
    }
private:
    inline static bool insideWork = false;
    inline static std::optional<std::invoke_result_t<Fn>> cachedParser;
};


template <auto tHash>
struct AutoTag {
    static constexpr auto value = tHash;
};

#define AutoTagT AutoTag<details::SourceLocation::current().hash()>
#define AutoTagV AutoTagT{}


template <typename Tag, std::invocable Fn>
auto lazyCached(Fn const& genParser) noexcept(std::is_nothrow_invocable_v<Fn>) {
    return make_parser(LazyCached<Tag, Fn>{genParser});
}

template <typename Tag, std::invocable Fn>
auto lazyCached(Fn const& genParser, Tag tag) noexcept(std::is_nothrow_invocable_v<Fn>) {
    return make_parser(LazyCached<Tag, Fn>{genParser});
}


template <typename T, typename tag>
class LazyForget {
    using InvokerType = typename Parser<T>::Result (*)(void*, Stream&);

    template <typename ParserOut>
    static InvokerType makeInvoker() noexcept {
        return [](void* pParser, Stream& s) {
            return static_cast<ParserOut*>(pParser)->apply(s);
        };
    }
public:
    template <typename Fn>
        requires(!std::is_same_v<Fn, LazyForget<T, tag>>)
    explicit LazyForget(Fn const& fn) noexcept(std::is_nothrow_invocable_v<Fn>) {
        if (pInvoke == nullptr) {
            using ParserOut = std::invoke_result_t<Fn>;
            pInvoke = makeInvoker<ParserOut>();
            pParser = new ParserOut(fn());
        }
    }

    auto operator()(Stream& stream) const {
        return pInvoke(pParser, stream);
    }
private:
    static inline InvokerType pInvoke = nullptr;
    static inline void* pParser = nullptr;
};


template <typename T, typename Ctx, typename tag>
        requires (!IsVoidCtx<Ctx>)
class LazyForgetCtx {
    using InvokerType = typename Parser<T, Ctx>::Result (*)(void*, Stream&, Ctx&);

    template <typename ParserOut>
    static InvokerType makeInvoker() noexcept {
        return [](void* pParser, Stream& s, auto& ctx) {
            return static_cast<ParserOut*>(pParser)->apply(s, ctx);
        };
    }
public:
    template <typename Fn>
        requires(!std::is_same_v<Fn, LazyForgetCtx<T, Ctx, tag>>)
    explicit LazyForgetCtx(Fn const& fn) noexcept(std::is_nothrow_invocable_v<Fn>) {
        if (pInvoke == nullptr) {
            using ParserOut = std::invoke_result_t<Fn>;
            pInvoke = makeInvoker<ParserOut>();
            pParser = new ParserOut(fn());
        }
    }

    auto operator()(Stream& stream, Ctx& ctx) const {
        return pInvoke(pParser, stream, ctx);
    }
private:
    static inline InvokerType pInvoke = nullptr;
    static inline void* pParser = nullptr;
};


template <typename T, typename Tag, typename Fn>
auto lazyForget(Fn const& f, Tag = {}) noexcept {
    return Parser<T>::make(LazyForget<T, Tag>{f});
}

template <typename T, typename Ctx, typename Fn, typename Tag>
auto lazyForgetCtx(Fn const& f, Tag = {}) noexcept {
    return Parser<T>::make(LazyForgetCtx<T, Ctx, Tag>{f});
}


struct LazyBindingTag{};
template <typename T, ContextType ParserCtx, typename Tag = LazyBindingTag>
class LazyCtxBinding {
public:
    struct LazyContext;

    static constexpr bool nocontext = false;
    using Ctx = UnionCtx<ParserCtx, ContextWrapper<LazyContext const&>>;

    using P = Parser<T, Ctx>;
    using ParserResult = T;

    struct LazyContext {
        explicit LazyContext(P const*const p) : parser(p) {};
        P const*const parser;
    };

    explicit LazyCtxBinding() noexcept = default;

    template <ContextType Context>
    auto operator()(Stream& stream, Context& ctx) const {
        auto const*const parser = get<LazyContext>(ctx).parser;
        return parser->apply(stream, ctx);
    }
};

template <typename Fn, typename Tag>
auto lazyCtxBindingFn(Fn const& f [[maybe_unused]], Tag = {}) noexcept {
    using P = std::invoke_result_t<Fn>;
    using T = GetParserResult<P>;
    using Ctx = GetParserCtx<P>;
    using LZ = LazyCtxBinding<T, Ctx, Tag>;
    return Parser<T, typename LZ::Ctx>::make(LZ{});
}


template <typename Tag, typename Fn>
auto lazyCtxBindingFn(Fn const& f [[maybe_unused]]) noexcept {
    return lazyCtxBinding(f, Tag{});
}


template <typename T, typename Tag = LazyBindingTag>
auto lazyCtxBinding() noexcept {
    using LZ = LazyCtxBinding<T, VoidContext, Tag>;
    return Parser<T, typename LZ::Ctx>::make(LZ{});
}


template <typename T, typename Tag>
auto lazyCtxBinding(Tag) noexcept {
    return lazyCtxBinding<T, Tag>();
}

template <typename T, typename Ctx, typename Tag = LazyBindingTag>
auto lazyCtxBindingCtx() noexcept {
    using LZ = LazyCtxBinding<T, Ctx, Tag>;
    return Parser<T, typename LZ::Ctx>::make(LZ{});
}

template <typename T, typename Ctx, typename Tag>
auto lazyCtxBindingCtx(Tag = Tag{}) noexcept {
    return lazyCtxBindingCtx<T, Ctx, Tag>();
}

template <typename T, ContextType Ctx, typename Tag = LazyBindingTag>
requires (sizeCtx<Ctx> == 1)
auto makeBindingCtx(Parser<T, Ctx> const& parser) noexcept {
    using LazyCtx = typename LazyCtxBinding<T, VoidContext, Tag>::LazyContext;
    return LazyCtx{std::addressof(parser)};
}

}
// #include <parsecpp/core/lift.h>


// #include <parsecpp/core/parser.h>

// #include <parsecpp/common/base.h>


// #include <parsecpp/core/parser.h>

// #include <parsecpp/utils/cmp.h>


#include <utility>


namespace prs::details {

template <typename T, typename First, typename ...Args>
constexpr bool cmpAnyOf(T const& t, First &&f, Args &&...args) noexcept {
    if constexpr (sizeof...(args) > 0) {
        return f == t || cmpAnyOf(t, std::forward<Args>(args)...);
    } else {
        return f == t;
    }
}

}

namespace prs {

/**
 *
 * @return Parser<T>
 */
template <typename T>
auto pure(T tValue) noexcept {
    using Value = std::decay_t<T>;
    return Parser<Value>::make([t = std::move(tValue)](Stream&) {
        return Parser<Value>::data(t);
    });
}

/**
 *
 * @return Parser<Unit>
 */
inline auto success() noexcept {
    return pure(Unit{});
}

/**
 *
 * @return Parser<Unit>
 */
inline auto fail() noexcept {
    return Parser<Unit>::make([](Stream& s) {
        return Parser<Unit>::makeError("Fail", s.pos());
    });
}

/**
 *
 * @return Parser<Unit>
 */
inline auto fail(std::string const& text) noexcept {
    return Parser<Unit>::make([text](Stream& s) {
        return Parser<Unit>::makeError(text, s.pos());
    });
}

}

namespace prs {


template <typename Ctx = VoidContext, typename Fn>
auto make_parser(Fn &&f) noexcept {
    using T = typename std::conditional_t<std::is_invocable_v<Fn, Stream&>
            , std::invoke_result<Fn, Stream&>
            , std::invoke_result<Fn, Stream&, Ctx&>>::type::Body;

    return Parser<T, Ctx>::make(std::forward<Fn>(f));
}

namespace details {

template <typename Fn, typename TupleParser, typename ...Values>
auto liftRec(Fn const& fn, Stream& stream, TupleParser const& parsers, Values &&...values) noexcept {
    constexpr size_t Ind = sizeof...(values);
    if constexpr (std::tuple_size_v<TupleParser> == Ind) {
        using ReturnType = std::invoke_result_t<Fn, Values...>;
        return Parser<ReturnType>::data(std::invoke(fn, values...));
    } else {
        return std::get<Ind>(parsers).apply(stream).flatMap([&](auto &&a) {
            return liftRec(
                    fn, stream, parsers, std::forward<Values>(values)..., a);
        });
    }
}


template <typename Fn, typename Ctx, typename TupleParser, typename ...Values>
auto liftRecCtx(Fn const& fn, Stream& stream, Ctx& ctx, TupleParser const& parsers, Values &&...values) noexcept {
    constexpr size_t Ind = sizeof...(values);
    if constexpr (std::tuple_size_v<TupleParser> == Ind) {
        using ReturnType = std::invoke_result_t<Fn, Values...>;
        if constexpr (std::is_invocable_v<Fn, Values...>) {
            return Parser<ReturnType>::data(std::invoke(fn, values...));
        } else {
            return Parser<ReturnType>::data(std::invoke(fn, values..., ctx));
        }
    } else {
        return std::get<Ind>(parsers).apply(stream, ctx).flatMap([&](auto &&a) {
            return liftRecCtx(
                    fn, stream, ctx, parsers, std::forward<Values>(values)..., a);
        });
    }
}

}

template <typename Fn, ParserType ...Args>
    requires(IsVoidCtx<GetContextTrait<Fn>> && (IsVoidCtx<GetParserCtx<Args>> && ...))
auto liftM(Fn fn, Args &&...args) noexcept {
    return make_parser([fn, parsers = std::make_tuple(args...)](Stream& s) {
        return details::liftRec(fn, s, parsers);
    });
}


template <typename Fn, ParserType ...Args>
    requires(!IsVoidCtx<GetContextTrait<Fn>> || (!IsVoidCtx<GetParserCtx<Args>> || ...))
auto liftM(Fn fn, Args &&...args) noexcept {
    using Ctx = UnionCtx<GetContextTrait<Fn>, GetParserCtx<Args>...>;
    return make_parser<Ctx>([fn, parsers = std::make_tuple(args...)](Stream& s, auto& ctx) {
        return details::liftRecCtx(fn, s, ctx, parsers);
    });
}

template <ParserType ...Args>
auto concat(Args &&...args) noexcept {
    return liftM(details::MakeTuple{}, std::forward<Args>(args)...);
}

template <typename Fn>
constexpr auto satisfy(Fn&& tTest) noexcept {
    return Parser<char>::make([test = std::forward<Fn>(tTest)](Stream& stream) {
        if (auto c = stream.checkFirst(test); c != 0) {
            return Parser<char>::data(c);
        } else {
            return Parser<char>::PRS_MAKE_ERROR("satisfy", stream.pos());
        }
    });
}

}
// #include <parsecpp/core/modifier.h>


// #include "parser.h"


namespace prs {

template <typename T>
class ModifyCallerI {
public:
    using Type = T;
    virtual details::ResultType<T> operator()() = 0;
};

template <ParserType Parser>
class ModifyCaller : public ModifyCallerI<GetParserResult<Parser>> {
public:
    using Type = typename ModifyCallerI<GetParserResult<Parser>>::Type;

    ModifyCaller(Parser const& p, Stream& s) noexcept
        : m_parser(p)
        , m_stream(s) {}

    typename Parser::Result operator()() final {
        return m_parser(m_stream);
    }
private:
    Parser const& m_parser;
    Stream& m_stream;
};


template <ParserType Parser, ContextType StoredCtx>
class ModifyCallerCtx : public ModifyCallerI<GetParserResult<Parser>> {
public:
    ModifyCallerCtx(Parser const& p, Stream& s, StoredCtx& ctx) noexcept
        : m_parser(p)
        , m_stream(s)
        , m_ctx(ctx) {

    }

    typename Parser::Result operator()() final {
        return m_parser(m_stream, m_ctx);
    }
private:
    Parser const& m_parser;
    Stream& m_stream;
    StoredCtx& m_ctx;
};

template <typename ModifierClass, typename Ctx>
class ModifyWithContext {
public:
    explicit ModifyWithContext(ModifierClass fn) noexcept(std::is_nothrow_move_constructible_v<ModifierClass>)
        : m_modifier(std::move(fn)) {

    }

    template <typename ...Args>
    explicit ModifyWithContext(Args&& ...args) noexcept(std::is_nothrow_constructible_v<ModifierClass, Args...>)
        : m_modifier(std::forward<Args>(args)...) {

    }

    auto operator()(auto& caller, Stream& stream, auto& ctx) const noexcept {
        return m_modifier(caller, stream, ctx);
    }
private:
    ModifierClass m_modifier;
};


template <ParserType ParserA, typename Modify>
    requires(ParserA::nocontext && std::is_invocable_v<Modify, ModifyCallerI<GetParserResult<ParserA>>&, Stream&>)
auto operator*(ParserA parserA, Modify modifier) noexcept {
    return make_parser([parser = std::move(parserA), mod = std::move(modifier)](Stream& stream) {
        ModifyCaller p{parser, stream};
        return mod(p, stream);
    });
}


template <ParserType ParserA, typename Modify>
    requires(!ParserA::nocontext)
auto operator*(ParserA parserA, Modify modifier) noexcept {
    using Ctx = GetParserCtx<ParserA>;
    return make_parser<Ctx>(
            [parser = std::move(parserA), mod = std::move(modifier)](Stream& stream, auto& ctx) {
        ModifyCallerCtx p{parser, stream, ctx};
        if constexpr (std::is_invocable_v<Modify, ModifyCallerI<GetParserResult<ParserA>>&, Stream&>) {
            return mod(p, stream);
        } else {
            return mod(p, stream, ctx);
        }
    });
}


template <ParserType ParserA, typename Modify, typename Ctx>
auto operator*(ParserA parserA, ModifyWithContext<Modify, Ctx> modifier) noexcept {
    using UCtx = UnionCtx<GetParserCtx<ParserA>, Ctx>;
    return make_parser<UCtx>(
            [parser = std::move(parserA), mod = std::move(modifier)](Stream& stream, auto& ctx) {
        if constexpr (ParserA::nocontext) {
            ModifyCaller p{parser, stream};
            return mod(p, stream, ctx);
        } else {
            ModifyCaller p{parser, stream, ctx};
            return mod(p, stream, ctx);
        }
    });
}

template <ParserType ParserA, typename Modify>
    requires (!IsVoidCtx<typename ContextTrait<Modify>::Ctx> && ParserA::nocontext)
auto operator*(ParserA parserA, Modify modifier) noexcept {
    using Ctx = typename ContextTrait<Modify>::Ctx;
    using UCtx = UnionCtx<GetParserCtx<ParserA>, Ctx>;
    return make_parser<UCtx>(
            [parser = std::move(parserA), mod = std::move(modifier)](Stream& stream, auto& ctx) {
        if constexpr (ParserA::nocontext) {
            ModifyCaller p{parser, stream};
            return mod(p, stream, ctx);
        } else {
            ModifyCaller p{parser, stream, ctx};
            return mod(p, stream, ctx);
        }
    });
}


/*
 * Priority for operator*
 * a `op` b * mod === a `op` (b * mod)
 * a `op` b *= mod === (a `op` b) * mod
 * forall `op` != >>=
 */
template <ParserType ParserA, typename Modify>
auto operator*=(ParserA parserA, Modify modify) noexcept {
    return parserA * modify;
}

}

// #include <parsecpp/common/base.h>

// #include <parsecpp/common/map.h>


// #include <parsecpp/core/parser.h>

// #include <parsecpp/core/lift.h>


#include <map>


namespace prs {

/**
 * @tparam errorInTheMiddle - fail parser if key can be parsed and value cannot
 * Key := ParserKey::Type
 * Value := ParserValue::Type
 * @return Parser<std::map<Key, Value>>
 */
template <bool errorInTheMiddle = true
        , size_t maxIteration = MAX_ITERATION
        , ParserType ParserKey
        , ParserType ParserValue>
auto toMap(ParserKey key, ParserValue value) noexcept {
    using Key = GetParserResult<ParserKey>;
    using Value = GetParserResult<ParserValue>;
    using Map = std::map<Key, Value>;
    using UCtx = UnionCtx<GetParserCtx<ParserKey>, GetParserCtx<ParserValue>>;
    using P = Parser<Map, UCtx>;
    return P::make([key, value](Stream& stream, auto& ctx) {
        Map out{};
        size_t iteration = 0;

        [[maybe_unused]]
        auto backup = stream.pos();
        do {
            auto keyRes = key.apply(stream, ctx);
            if (!keyRes.isError()) {
                auto valueRes = value.apply(stream, ctx);
                if (!valueRes.isError()) {
                    out.insert_or_assign(std::move(keyRes).data(), std::move(valueRes).data());
                } else {
                    if constexpr (errorInTheMiddle) {
                        return P::makeError("Parse key but cannot parse value", stream.pos());
                    } else {
                        stream.restorePos(backup);
                        return P::data(std::move(out));
                    }
                }
            } else {
                stream.restorePos(backup);
                return P::data(std::move(out));
            }

            backup = stream.pos();
        } while (++iteration != maxIteration);

        return P::makeError("Max iteration", stream.pos());
    });
}


/**
 * @tparam errorInTheMiddle - fail parser if key or delim can be parsed and value(key) cannot
 * Key := ParserKey::Type
 * Value := ParserValue::Type
 * @return Parser<std::map<Key, Value>>
 */
template <bool errorInTheMiddle = true
        , size_t maxIteration = MAX_ITERATION
        , ParserType ParserKey
        , ParserType ParserValue
        , ParserType ParserDelimiter>
auto toMap(ParserKey tKey, ParserValue tValue, ParserDelimiter tDelimiter) noexcept {
    using Key = GetParserResult<ParserKey>;
    using Value = GetParserResult<ParserValue>;
    using Map = std::map<Key, Value>;
    using UCtx = UnionCtx<GetParserCtx<ParserKey>, GetParserCtx<ParserValue>, GetParserCtx<ParserDelimiter>>;
    using P = Parser<Map, UCtx>;
    return P::make([key = std::move(tKey), value = std::move(tValue), delimiter = std::move(tDelimiter)](Stream& stream, auto& ctx) {
        Map out{};
        size_t iteration = 0;

        auto backup = stream.pos();
        do {
            auto keyRes = key.apply(stream, ctx);
            if (!keyRes.isError()) {
                auto valueRes = value.apply(stream, ctx);
                if (!valueRes.isError()) {
                    out.insert_or_assign(std::move(keyRes).data(), std::move(valueRes).data());
                } else {
                    if constexpr (errorInTheMiddle) {
                        return P::makeError("Parse key but cannot parse value", stream.pos());
                    } else {
                        stream.restorePos(backup);
                        return P::data(std::move(out));
                    }
                }
            } else {
                stream.restorePos(backup);
                return P::data(std::move(out));
            }

            backup = stream.pos();
        } while (!delimiter.apply(stream, ctx).isError() && ++iteration != maxIteration);

        if (iteration == maxIteration) {
            return P::makeError("Max iteration", stream.pos());
        } else {
            stream.restorePos(backup);
            return P::data(std::move(out));
        }
    });
}

}
// #include <parsecpp/common/number.h>


// #include <parsecpp/core/parser.h>

// #include <parsecpp/common/string.h>


// #include <parsecpp/core/parser.h>

// #include <parsecpp/core/lift.h>


// #include <parsecpp/utils/constexprString.hpp>


namespace prs {

namespace details {

template<std::size_t N>
struct MakeArray
{
    std::array<char, N> data;

    template <std::size_t... Is>
    constexpr MakeArray(char const(&arr)[N], std::integer_sequence<std::size_t, Is...>)
        : data{arr[Is]...} {

    }

    constexpr MakeArray(char const(&arr)[N])
        : MakeArray(arr, std::make_integer_sequence<std::size_t, N>()) {

    }

    constexpr auto size() const {
        return N;
    }
};

}

template<std::size_t N>
struct ConstexprString {
    std::array<char, N + 1> m_str;

    constexpr explicit ConstexprString(char const(&s)[N + 1]) noexcept {
        for (std::size_t i = 0; i <= N; ++i) {
            m_str[i] = s[i];
        }
    }

    constexpr explicit ConstexprString(std::array<char, N + 1> arr) noexcept
        : m_str(arr) {
    }

    constexpr explicit ConstexprString(details::MakeArray<N + 1> arr) noexcept{
        for (size_t i = 0; i != N + 1; ++i) {
            m_str[i] = arr.data[i];
        }
    }

    static constexpr ConstexprString<1> fromChar(char c) noexcept {
        return ConstexprString<1>({c});
    }

    constexpr const char* c_str() const noexcept {
        return m_str.data();
    }

    constexpr size_t size() const noexcept {
        return N;
    }

    constexpr std::string_view sv() const noexcept {
        return std::string_view(c_str(), size());
    }

    std::string toString() const noexcept {
        return std::string(c_str(), size());
    }

    template<std::size_t M>
    constexpr auto operator+(ConstexprString<M> const& other) const noexcept {
        std::array<char, N + M + 1> new_str{};
        for (std::size_t i = 0; i != N; ++i) {
            new_str[i] = m_str[i];
        }
        for (std::size_t i = 0; i != M; ++i) {
            new_str[N + i] = other.m_str[i];
        }
        return ConstexprString<N + M>(new_str);
    }

    constexpr auto add(char c) const noexcept {
        return operator+(fromChar(c));
    }

    constexpr auto between(char c) const noexcept {
        return fromChar(c) + *this + fromChar(c);
    }


    constexpr auto between(char l, char r) const noexcept {
        return fromChar(l) + *this + fromChar(r);
    }


    template <size_t M>
    constexpr bool operator==(ConstexprString<M> const& rhs) const noexcept {
        if constexpr (M != N) {
            return false;
        } else {
            for (auto i = 0; i != N; ++i) {
                if (rhs[i] != this->operator[](i)) {
                    return false;
                }
            }
            return true;
        }
    }

    template <size_t M>
    constexpr bool operator!=(ConstexprString<M> const& rhs) const noexcept {
        return !operator==(rhs);
    }

    constexpr auto operator[](size_t i) const noexcept {
        return m_str[i];
    }


    template <size_t M>
    constexpr ConstexprString<std::min(M, N)> substr() const noexcept {
        constexpr auto T = std::min(M, N);
        std::array<char, T + 1> out{};
        for (auto i = 0; i != T; ++i) {
            out[i] = operator[](i);
        }
        return ConstexprString<T>(out);
    }

    template <size_t M>
    requires(M <= N)
    constexpr bool startsFrom(ConstexprString<M> const& prefix) const noexcept {
        return substr<M>() == prefix;
    }
};

template <details::MakeArray arr>
constexpr auto operator""_prs() {
    return ConstexprString<arr.size() - 1>(arr);
}


}

namespace prs {

/**
 *
 * @return Parser<char>
 */
inline auto anyChar() noexcept {
    auto p = [](Stream& stream) {
        if (stream.eos()) {
            return Parser<char>::makeError("empty string", stream.pos());
        } else {
            char c = stream.front();
            stream.moveUnsafe();
            return Parser<char>::data(c);
        }
    };

    return make_parser(p);
}

/**
 *
 * @return Parser<Unit>
 */
inline auto spaces() noexcept {
    return make_parser([](Stream& str) {
        while (str.checkFirst([](char c) {
            return std::isspace(c);
        }));

        return prs::Parser<Unit>::data({});
    });
}


/**
 *
 * @return Parser<Unit>
 */
inline auto spacesFast() noexcept {
    return make_parser([](Stream& str) {
        while (str.checkFirst([](char c) {
            return c == ' ';
        }));

        return prs::Parser<Unit>::data({});
    });
}


/**
 *
 * @return Parser<StringType>
 */
template <bool allowDigit = false, typename StringType = std::string_view>
auto letters() noexcept {
    return make_parser([](Stream& str) {
        auto start = str.pos();
        while (str.checkFirst([](char c) {
            return ('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z') || (allowDigit && ('0' <= c && c <= '9'));
        }));

        auto end = str.pos();
        if (start == end) {
            return Parser<StringType>::makeError("Empty word", str.pos());
        } else {
            return Parser<StringType>::data(StringType{str.get_sv(start, end)});
        }
    });
}

class FromRange {
public:
    constexpr FromRange(char begin, char end) noexcept
        : m_begin(begin)
        , m_end(end) {

    }

    friend constexpr bool operator==(FromRange const& range, char c) noexcept {
        return range.m_begin <= c && c <= range.m_end;
    }

// no private to be structural and avoid next error:
// 'prs::FromRange' is not a valid type for a template non-type parameter because it is not structural
//private:
    char m_begin;
    char m_end;
};


template <typename T, typename U>
concept LeftCmpWith = requires(const std::remove_reference_t<T>& t,
        const std::remove_reference_t<U>& u) {
    {t == u} -> std::convertible_to<bool>; // boolean-testable
};

template <typename StringType = std::string_view, LeftCmpWith<char> ...Args>
auto lettersFrom(Args ...args) noexcept {
    static_assert(sizeof...(args) > 0);
    return make_parser([args...](Stream& str) {
        auto start = str.pos();
        while (str.checkFirst([&](char c) {
            return ((args == c) || ...);
        }));

        auto end = str.pos();
        return Parser<StringType>::data(StringType{str.get_sv(start, end)});
    });
}

template <auto ...args>
auto lettersFrom() noexcept {
    static_assert(sizeof...(args) > 0);
    using T = std::string_view;
    return make_parser([](Stream& str) {
        auto start = str.pos();
        while (str.checkFirst([&](char c) {
            return ((args == c) || ...);
        }));

        auto end = str.pos();
        return Parser<T>::data(str.get_sv(start, end));
    });
}

template <LeftCmpWith<char> ...Args>
auto skipChars(Args ...args) noexcept {
    static_assert(sizeof...(args) > 0);
    return make_parser([args...](Stream& str) {
        while (str.checkFirst([&](char c) {
            return ((args == c) || ...);
        }));

        return Parser<Drop>::data({});
    });
}



template <auto ...args>
auto skipChars() noexcept {
    static_assert(sizeof...(args) > 0);
    return make_parser([](Stream& str) {
        while (str.checkFirst([&](char c) {
            return ((args == c) || ...);
        }));

        return Parser<Drop>::data({});
    });
}


template <bool forwardSearch = false>
auto searchText(std::string const& searchPattern) noexcept {
    return Parser<Unit>::make([searchPattern](Stream& stream) {
        auto &str = stream.sv();
        if (auto pos = str.find(searchPattern); pos != std::string_view::npos) {
            if constexpr (forwardSearch) {
                stream.move(pos > 0 ? pos - 1 : 0);
                return Parser<Unit>::data({});
            } else {
                str = str.substr(pos + searchPattern.size());
                return Parser<Unit>::data({});
            }
        } else {
            return Parser<Unit>::PRS_MAKE_ERROR("Cannot find '" + std::string(searchPattern) + "'", stream.pos());
        }
    });
}

template <ConstexprString searchPattern, bool forwardSearch = false>
auto searchText() noexcept {
    return Parser<Unit>::make([](Stream& stream) {
        auto &str = stream.sv();
        if (auto pos = str.find(searchPattern.sv()); pos != std::string_view::npos) {
            if constexpr (forwardSearch) {
                stream.move(pos > 0 ? pos - 1 : 0);
                return Parser<Unit>::data({});
            } else {
                str = str.substr(pos + searchPattern.size());
                return Parser<Unit>::data({});
            }
        } else {
            return Parser<Unit>::PRS_MAKE_ERROR("Cannot find '" + searchPattern.toString() + "'", stream.pos());
        }
    });
}

template <LeftCmpWith<char> ...Args>
constexpr auto charFrom(Args ...chars) noexcept {
    return satisfy([=](char c) {
        return details::cmpAnyOf(c, chars...);
    });
}

template <auto ...chars>
constexpr auto charFrom() noexcept {
    return satisfy([](char c) {
        return details::cmpAnyOf(c, chars...);
    });
}


template <LeftCmpWith<char> ...Args>
auto charFromSpaces(Args ...chars) noexcept {
    return spaces() >> charFrom(std::forward<Args>(chars)...) << spaces();
}

template <typename StringType = std::string_view>
auto between(char borderLeft, char borderRight) noexcept {
    using P = Parser<StringType>;
    return P::make([borderLeft, borderRight](Stream& stream) {
        if (stream.checkFirst(borderLeft) == 0) {
            return P::makeError("No leftBorder", stream.pos());
        }

        auto start = stream.pos();
        while (stream.checkFirst([borderRight](char c) {
            return c != borderRight;
        }));

        auto ans = stream.full().substr(start, stream.pos() - start);
        if (stream.checkFirst(borderRight) == 0) {
            return P::makeError("No rightBorder", stream.pos());
        }

        return P::data(StringType{ans});
    });
}

template <typename StringType = std::string_view>
auto between(char border) noexcept {
    return between<StringType>(border, border);
}


template <ParserType Parser>
auto between(char borderLeft, char borderRight, Parser parser) noexcept {
    return charFrom(borderLeft) >> parser << charFrom(borderRight);
}


template <typename StringType = std::string_view, std::predicate<char> Fn>
auto until(Fn fn) noexcept {
    return Parser<StringType>::make([fn](Stream& stream) {
        auto start = stream.pos();
        while (stream.checkFirst([&](char c) {
            return !fn(c);
        }));

        auto end = stream.pos();
        return Parser<StringType>::data(StringType{stream.get_sv(start, end)});
    });
}

template <typename StringType = std::string_view, std::predicate<char, Stream&> Fn>
auto until(Fn fn) noexcept {
    return Parser<StringType>::make([fn](Stream& stream) {
        auto start = stream.pos();
        while (stream.checkFirst([&](char c) {
            return !fn(c, stream);
        }));

        auto end = stream.pos();
        return Parser<StringType>::data(StringType{stream.get_sv(start, end)});
    });
}


template <typename StringType = std::string_view, typename ...Args>
auto until(Args ...args) noexcept {
    return Parser<StringType>::make([args...](Stream& stream) {
        auto start = stream.pos();
        while (stream.checkFirst([&](char c) {
            return !((args == c) || ...);
        }));

        auto end = stream.pos();
        return Parser<StringType>::data(StringType{stream.get_sv(start, end)});
    });
}


template <auto ...args>
auto until() noexcept {
    using StringType = std::string_view;
    return Parser<StringType>::make([](Stream& stream) {
        auto start = stream.pos();
        while (stream.checkFirst([&](char c) {
            return !((args == c) || ...);
        }));

        auto end = stream.pos();
        return Parser<StringType>::data(StringType{stream.get_sv(start, end)});
    });
}


template <ParserType Parser>
auto between(char border, Parser parser) noexcept {
    return between(border, border, std::move(parser));
}


template <typename StringType = std::string_view>
auto literal(std::string str) noexcept {
    return Parser<StringType>::make([str](Stream& s) {
        if (s.sv().starts_with(str)) {
            s.move(str.size());
            return Parser<StringType>::data(StringType{str});
        } else {
            return Parser<StringType>::makeError("Cannot find literal", s.pos());
        }
    });
}

template <ConstexprString str>
auto literal() noexcept {
    using StringType = std::string_view;
    return Parser<StringType>::make([](Stream& s) {
        if (s.sv().starts_with(str)) {
            s.move(str.size());
            return Parser<StringType>::data(str);
        } else {
            return Parser<StringType>::makeError("Cannot find literal", s.pos());
        }
    });
}


template <char endSymbol, char escapingSymbol = '\\'>
auto escapedString() noexcept {
    if constexpr (endSymbol != escapingSymbol) {
        return Parser<std::string>::make([](Stream& s) {
            bool isEscaped = false;
            const auto sv = s.sv();
            std::string out;
            for (size_t i = 0; i != sv.size(); ++i) {
                switch (sv[i]) {
                    case escapingSymbol: {
                        if (std::exchange(isEscaped, !isEscaped)) {
                            out.push_back(escapingSymbol);
                        }
                        break;
                    }
                    case endSymbol: {
                        if (isEscaped) {
                            isEscaped = false;
                            out.push_back(endSymbol);
                        } else {
                            s.moveUnsafe(i + 1);
                            return Parser<std::string>::data(std::move(out));
                        }
                        break;
                    }
                    default: out.push_back(sv[i]);
                }
            }

            return Parser<std::string>::makeError("Cannot find end symbol", s.pos());
        });
    } else {
        return Parser<std::string>::make([](Stream& s) {
            bool isEscaped = false;
            const auto sv = s.sv();
            std::string out;
            for (size_t i = 0; i != sv.size(); ++i) {
                if (sv[i] == endSymbol) {
                    if (isEscaped) {
                        isEscaped = false;
                        out.push_back(endSymbol);
                    } else {
                        if (i + 1 != sv.size() && sv[i + 1] == endSymbol) {
                            isEscaped = true;
                        } else {
                            s.moveUnsafe(i + 1);
                            return Parser<std::string>::data(std::move(out));
                        }
                    }
                } else {
                    out.push_back(sv[i]);
                }
            }

            return Parser<std::string>::makeError("Cannot find end symbol", s.pos());
        });
    }
}

}

#include <charconv>


namespace prs {

namespace {

template <typename Number, typename = std::void_t<>>
constexpr bool hasFromCharsMethod = false;

template <typename Number>
constexpr bool hasFromCharsMethod<Number, std::void_t<decltype(std::from_chars(nullptr, nullptr, std::declval<Number&>()))>> = true;

}

/**
 * @return Parser<Number>
 */
template <typename Number = double>
    requires (std::is_arithmetic_v<Number>)
auto number() noexcept {
    return Parser<Number>::make([](Stream& s) {
        auto sv = s.sv();
        Number n{};
        if constexpr (hasFromCharsMethod<Number>) {
            if (auto res = std::from_chars(sv.data(), sv.data() + sv.size(), n);
                    res.ec == std::errc{}) {
                s.moveUnsafe(res.ptr - sv.data());
                return Parser<Number>::data(n);
            } else {
                // TODO: get error and pos from res
                return Parser<Number>::makeError("Cannot parse number", s.pos());
            }
        } else { // some system doesn't have from_chars for floating point numbers
            if constexpr (std::is_same_v<Number, double>) {
                char *end = const_cast<char*>(sv.end());
                n = std::strtod(sv.data(), &end);
                size_t endIndex = end - sv.data();
                if (endIndex == 0) {
                    return Parser<Number>::makeError("Cannot parse number", s.pos());
                }
                if (errno == ERANGE) {
                    return Parser<Number>::makeError("Cannot parse number ERANGE", s.pos());
                }
                s.moveUnsafe(endIndex);
                return Parser<Number>::data(n);
            } else if (std::is_same_v<Number, float>) {
                char *end = const_cast<char*>(sv.end());
                n = std::strtof(sv.data(), &end);
                size_t endIndex = end - sv.data();
                if (endIndex == 0) {
                    return Parser<Number>::makeError("Cannot parse number", s.pos());
                }
                if (errno == ERANGE) {
                    return Parser<Number>::makeError("Cannot parse number ERANGE", s.pos());
                }
                s.moveUnsafe(endIndex);
                return Parser<Number>::data(n);
            } else {
                static_assert(!std::is_void_v<Number>, "Number type doesn't support");
                return Parser<Number>::makeError("Not supported", s.pos());
            }
        }
    });
}


/**
 *
 * @return Parser<char>
 */
constexpr auto digit() noexcept {
    return charFrom(FromRange('0', '9'));
}


inline auto digits() noexcept {
    return lettersFrom<FromRange('0', '9')>();
}

}
// #include <parsecpp/common/string.h>

// #include <parsecpp/common/process.h>


// #include <parsecpp/core/parser.h>

// #include <parsecpp/core/modifier.h>


namespace prs {

/**
 * skipNext :: Parser<A> -> Parser<Skip> -> Parser<A>
 */
template <ParserType ParserData, ParserType Skip>
auto skipToNext(ParserData data, Skip skip) noexcept {
    using UCtx = UnionCtx<GetParserCtx<ParserData>, GetParserCtx<Skip>>;
    if constexpr (IsVoidCtx<UCtx>) {
        return ParserData::make([data, skip](Stream& stream) {
            do {
                auto result = data(stream);
                if (!result.isError()) {
                    return result;
                }
            } while (!skip(stream).isError());
            return ParserData::makeError("Cannot find next item", stream.pos());
        });
    } else {
        return Parser<GetParserResult<ParserData>, UCtx>::make([data, skip](Stream& stream, UCtx& ctx) {
            do {
                auto result = data(stream, ctx);
                if (!result.isError()) {
                    return std::move(result);
                }
            } while (!skip(stream, ctx).isError());
            return ParserData::makeError("Cannot find next item", stream.pos());
        });
    }
}


/**
 * search :: Parser<A> -> Parser<A>
 */
template <ParserType P>
    requires (IsVoidCtx<GetParserCtx<P>>)
auto search(P tParser) noexcept {
    return P::make([parser = std::move(tParser)](Stream& stream) {
        auto start = stream.pos();
        auto searchPos = start;
        while (!stream.eos()) {
            if (decltype(auto) result = parser.apply(stream); !result.isError()) {
                return P::data(std::move(result).data());
            } else {
                stream.restorePos(++searchPos);
            }
        }

        stream.restorePos(start);
        return P::makeError("Cannot find", stream.pos());
    });
}


template <ParserType P>
    requires (!IsVoidCtx<GetParserCtx<P>>)
auto search(P tParser) noexcept {
    return P::make([parser = std::move(tParser)](Stream& stream, auto& ctx) {
        auto start = stream.pos();
        auto searchPos = start;
        while (!stream.eos()) {
            if (decltype(auto) result = parser.apply(stream, ctx); !result.isError()) {
                return P::data(std::move(result).data());
            } else {
                stream.restorePos(++searchPos);
            }
        }

        stream.restorePos(start);
        return P::makeError("Cannot find", stream.pos());
    });
}


template <typename Derived, typename Container, ContextType Ctx>
class Repeat {
public:
    Repeat() noexcept = default;

    auto operator()(auto& parser, Stream& stream, Ctx& ctx) {
        using P = Parser<typename std::decay_t<decltype(parser)>::Type, Ctx>;

        get().init();
        size_t iteration = 0;
        auto backup = stream.pos();
        do {
            backup = stream.pos();
            auto result = parser();
            if (!result.isError()) {
                get().add(std::move(result).data(), ctx);
            } else {
                stream.restorePos(backup);
                return P::data(get().value());
            }
        } while (++iteration != MAX_ITERATION);

        return P::makeError("Max iteration", stream.pos());
    }

private:
    Derived& get() noexcept {
        return static_cast<Derived&>(*this);
    }
};


template <typename Derived, typename ContainerT>
class Repeat<Derived, ContainerT, VoidContext> {
public:
    using Container = ContainerT;

    Repeat() noexcept = default;

    auto operator()(auto& parser, Stream& stream) const {
        using P = Parser<Container>;

        decltype(auto) container = get().init();
        size_t iteration = 0;
        do {
            auto backup = stream.pos();
            auto result = parser();
            if (!result.isError()) {
                get().add(container, std::move(result).data());
            } else {
                stream.restorePos(backup);
                return P::data(std::move(container));
            }
        } while (++iteration != MAX_ITERATION);

        return P::makeError("Max iteration", stream.pos());
    }

protected:
    Container init() const noexcept {
        return {};
    }

private:
    Derived const& get() const noexcept {
        return static_cast<Derived const&>(*this);
    }
};


template <typename Fn>
class ProcessRepeat : public Repeat<ProcessRepeat<Fn>, Unit, VoidContext> {
public:
    explicit ProcessRepeat(Fn fn) noexcept(std::is_nothrow_move_constructible_v<Fn>)
        : m_fn(std::move(fn)) {

    }


    template <typename T>
    void add(Unit, T&& t) const noexcept(std::is_nothrow_invocable_v<Fn, T>) {
        m_fn(std::forward<T>(t));
    }

private:
    Fn m_fn;
};


template <typename Fn>
auto processRepeat(Fn fn) {
    return ProcessRepeat<std::decay_t<Fn>>{fn};
}

template <ParserType P>
struct ConvertResult {
public:
    using Ctx = GetParserCtx<P>;

    explicit ConvertResult(P parser) noexcept
        : m_parser(std::move(parser)) {

    }

    auto operator()(auto& parser, Stream& stream) const requires (IsVoidCtx<Ctx>) {
        using ParserResult = typename std::decay_t<decltype(parser)>::Type;
        static_assert(std::is_constructible_v<Stream, ParserResult>);
        return parser().flatMap([&](auto &&t) {
            Stream localStream{t};
            return m_parser(localStream).flatMapError([&](details::ParsingError const& error) {
                auto posOfError = stream.pos() - (localStream.full().size() - error.pos);
                return P::PRS_MAKE_ERROR("Internal parser fail: " + error.description, posOfError);
            });
        });
    }

    auto operator()(auto& parser, Stream& stream, Ctx& ctx) const {
        using ParserResult = typename std::decay_t<decltype(parser)>::Type;
        static_assert(std::is_constructible_v<Stream, ParserResult>);
        return parser().flatMap([&](auto &&t) {
            Stream localStream{t};
            return m_parser(localStream, ctx).flatMapError([&](details::ParsingError const& error) {
                auto posOfError = stream.pos() - (localStream.full().size() - error.pos);
                return Parser<ParserResult>::PRS_MAKE_ERROR("Internal parser fail: " + error.description, posOfError);
            });
        });
    }
private:
    P m_parser;
};


template <ParserType P>
auto convertResult(P parser) noexcept {
    return ConvertResult{parser};
}

}
