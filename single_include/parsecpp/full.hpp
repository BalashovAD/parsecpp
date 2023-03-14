// #include <parsecpp/all.hpp>


// #include <parsecpp/core/parser.h>


// #include <parsecpp/core/expected.h>


// #include "parsecpp/utils/funcHelper.h"


// #include <parsecpp/core/stream.h>


// #include <parsecpp/core/parsingError.h>


// #include <parsecpp/core/buildParams.h>


namespace prs::details {

#ifndef PRS_DISABLE_ERROR_LOG
static constexpr bool DISABLE_ERROR_LOG = false;
#define PRS_MAKE_ERROR(strError, pos) makeError(strError, pos);
#else
static constexpr bool DISABLE_ERROR_LOG = true;
#define PRS_MAKE_ERROR(strError, pos) makeError("", pos);
#endif

}
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


#include <string>

namespace prs::details {

struct ParsingError {
    std::string description;
    size_t pos{};
};


}
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
        if constexpr (details::DISABLE_ERROR_LOG) {
            stream << ", dsc: optimized";
        } else {
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

#include <functional>
#include <memory>

#include <string>


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


template <typename Parser>
using parser_result_t = typename Parser::Type;


template <typename Parser>
constexpr bool is_parser_v = std::is_invocable_v<Parser, Stream&>;

template<typename T>
concept ParserType = is_parser_v<T>;

static constexpr size_t MAX_ITERATION = 1000000;

namespace details {

struct Id {
    template<typename T>
    T operator()(T t) const noexcept {
        return t;
    }


    template<typename T>
    T operator()(T const& t) const noexcept {
        return t;
    }
};

struct ToString {
    template<typename T>
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
    auto operator()(Args &&...args) const noexcept {
        return T{args...};
    }
};

template <typename T>
struct MakeShared {
    template <class ...Args>
    auto operator()(Args &&...args) const noexcept {
        return std::make_shared<T>(args...);
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

} // details
} // prs

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
            new (&m_error)T(e.m_error);
        } else {
            new (&m_data)Error(e.m_data);
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

        using Result = std::invoke_result_t<OnSuccess, const T&>;
        return isError() ? Expected<Result, Error>{m_error} : Expected<Result, Error>{onSuccess(m_data)};
    }


    template<std::invocable<T> OnSuccess>
    auto map(OnSuccess onSuccess) &&
                    noexcept(map_move_nothrow<OnSuccess>) {

        using Result = std::invoke_result_t<OnSuccess, const T&>;
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

// #include <parsecpp/core/parsingError.h>

// #include <parsecpp/core/stream.h>


#include <concepts>
#include <string_view>
#include <functional>
#include <utility>
#include <optional>
#include <glob.h>


namespace prs {

namespace details {

template <class Body>
using ResultType = Expected<Body, ParsingError>;

template <typename T>
using StdFunction = std::function<ResultType<T>(Stream&)>;

}

template <typename T, typename Func = details::StdFunction<T>>
        requires std::invocable<Func, Stream&>
class Parser {
public:
    static constexpr bool nothrow = std::is_nothrow_invocable_v<Func, Stream&>;

    using StoredFn = std::decay_t<Func>;

    using Type = T;
    using Result = details::ResultType<T>;

    template <std::invocable<Stream&> Fn>
    static auto make(Fn &&f) noexcept {
        return Parser<T, Fn>(std::forward<Fn>(f));
    }


    template <typename U>
        requires(std::is_same_v<std::decay_t<U>, StoredFn>)
    explicit Parser(U&& f) noexcept
        : m_fn(std::forward<U>(f)) {

    }

    Result operator()(Stream& stream) const noexcept(nothrow) {
        return std::invoke(m_fn, stream);
    }


    Result apply(Stream& stream) const noexcept(nothrow) {
        return operator()(stream);
    }

    template <typename B, typename Rhs>
    auto operator>>(Parser<B, Rhs> rhs) const noexcept {
        return Parser<B>::make([lhs = *this, rhs](Stream& stream) noexcept(nothrow && Parser<B, Rhs>::nothrow) {
            return lhs.apply(stream).flatMap([&rhs, &stream](T const& body) noexcept(Parser<B, Rhs>::nothrow) {
                return rhs.apply(stream);
            });
        });
    }

    template <typename B, typename Rhs>
    auto operator<<(Parser<B, Rhs> rhs) const noexcept {
        constexpr bool firstCallNoexcept = nothrow && Parser<B, Rhs>::nothrow;
        return Parser<T>::make([lhs = *this, rhs](Stream& stream) noexcept(firstCallNoexcept) {
            return lhs.apply(stream).flatMap([&rhs, &stream](T body) noexcept(Parser<Rhs>::nothrow) {
                return rhs.apply(stream).map([mvBody = std::move(body)](auto const& _) {
                    return mvBody;
                });
            });
        });
    }


    /*
     * <$>, fmap operator
     */
    template <std::invocable<T> ListFn>
    friend auto operator>>=(Parser lhs, ListFn fn) noexcept {
        return Parser<std::invoke_result_t<ListFn, T>>::make([lhs, fn](Stream& str) {
           return lhs.apply(str).map(fn);
        });
    }


    template <typename Rhs>
    auto operator|(Parser<T, Rhs> rhs) const noexcept {
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


    template <typename A>
    using MaybeValue = std::conditional_t<std::is_same_v<A, Drop>, Drop, std::optional<A>>;

    auto maybe() const noexcept {
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
    }


    auto maybeOr(T const& defaultValue) const noexcept {
        return Parser<T>::make([parser = *this, defaultValue](Stream& stream) {
            auto backup = stream.pos();
            return parser.apply(stream).flatMapError([&stream, &backup, &defaultValue](details::ParsingError const& error) {
                stream.restorePos(backup);
                return data(defaultValue);
            });
        });
    }


    template <size_t reserve = 0, size_t maxIteration = MAX_ITERATION>
            requires(!std::is_same_v<T, Drop>)
    auto repeat() const noexcept {
        using Value = T;
        using P = Parser<std::vector<Value>>;
        return P::make([value = *this](Stream& stream) {
            std::vector<Value> out{};
            out.reserve(reserve);

            size_t iteration = 0;

            auto backup = stream.pos();
            do {
                backup = stream.pos();
                auto result = value.apply(stream);
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

    template <size_t maxIteration = MAX_ITERATION>
            requires(std::is_same_v<T, Drop>)
    auto repeat() const noexcept {
        using P = Parser<Drop>;
        return P::make([value = *this](Stream& stream) noexcept(nothrow) {
            size_t iteration = 0;

            auto backup = stream.pos();
            do {
                backup = stream.pos();
                auto result = value.apply(stream);
                if (result.isError()) {
                    stream.restorePos(backup);
                    return P::data({});
                }
            } while (++iteration != maxIteration);

            return P::makeError("Max iteration", stream.pos());
        });
    }


    template <size_t reserve = 0, size_t maxIteration = MAX_ITERATION, ParserType Delimiter>
            requires(!std::is_same_v<T, Drop>)
    auto repeat(Delimiter tDelimiter) const noexcept {
        using Value = T;
        using P = Parser<std::vector<Value>>;
        constexpr bool noexceptP = nothrow && Delimiter::nothrow;
        return P::make([value = *this, delimiter = std::move(tDelimiter)](Stream& stream) noexcept(noexceptP) {
            std::vector<Value> out{};
            out.reserve(reserve);

            size_t iteration = 0;

            auto backup = stream.pos();
            do {
                auto result = value.apply(stream);
                if (!result.isError()) {
                    out.emplace_back(std::move(result).data());
                } else {
                    return P::data(std::move(out));
                }

                backup = stream.pos();
            } while (!delimiter.apply(stream).isError() && ++iteration != maxIteration);

            if (iteration == maxIteration) {
                return P::makeError("Max iteration", stream.pos());
            } else {
                stream.restorePos(backup);
                return P::data(std::move(out));
            }
        });
    }


    template <size_t maxIteration = MAX_ITERATION, ParserType Delimiter>
            requires(std::is_same_v<T, Drop>)
    auto repeat(Delimiter tDelimiter) const noexcept {
        using P = Parser<Drop>;
        return P::make([value = *this, delimiter = std::move(tDelimiter)](Stream& stream) {
            size_t iteration = 0;

            auto backup = stream.pos();
            do {
                auto result = value.apply(stream);
                if (result.isError()) {
                    return P::data(Drop{});
                }

                backup = stream.pos();
            } while (!delimiter.apply(stream).isError() && ++iteration != maxIteration);

            if (iteration == maxIteration) {
                return P::makeError("Max iteration", stream.pos());
            } else {
                stream.restorePos(backup);
                return P::data(Drop{});
            }
        });
    }


    template <std::predicate<T const&> Fn>
            requires (!std::predicate<T const&, Stream&>)
    auto cond(Fn test) const noexcept {
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

    template <std::predicate<T const&, Stream&> Fn>
    auto cond(Fn test) const noexcept {
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

    auto drop() const noexcept {
        return Parser<Drop>::make([p = *this](Stream& s) {
            return p.apply(s).map([](auto &&) {
                return Drop{};
            });
        });
    }

    auto endOfStream() const noexcept {
        return cond([](T const& t, Stream& s) {
            return s.eos();
        });
    }

    auto mustConsume() const noexcept {
        return make([p = *this](Stream& s) {
            auto pos = s.pos();
            return p.apply(s).flatMap([pos, &s](auto &&t) {
               if (s.pos() > pos) {
                   return data(t);
               } else {
                   return makeError("Didn't consume stream", s.pos());
               }
            });
        });
    }


    template <typename Fn>
    auto flatMap(Fn fn) const noexcept(std::is_nothrow_invocable_v<Fn, T const&>) {
        return make_parser([parser = *this, fn](Stream& stream) {
            return parser.apply(stream).flatMap([fn, &stream](T const& t) {
                return fn(t).apply(stream);
            });
        });
    }

    auto toCommonType() const noexcept {
        details::StdFunction<T> f = [fn = m_fn](Stream& stream) {
            return std::invoke(fn, stream);
        };

        return Parser<T>(f);
    }

    static Result makeError(details::ParsingError error) noexcept {
        return Result{error};
    }

    static Result makeError(size_t pos) noexcept {
        return Result{details::ParsingError{"", pos}};
    }

#ifdef PRS_DISABLE_ERROR_LOG
    static Result makeError(std::string_view desc, size_t pos) noexcept {
        return Result{details::ParsingError{"", pos}};
    }
#else
    static Result makeError(
            std::string desc,
            size_t pos,
            details::SourceLocation source = details::SourceLocation::current()) noexcept {
        return Result{details::ParsingError{std::move(desc), pos}};
    }
#endif

    template <typename U = T>
        requires(std::convertible_to<U, T>)
    static Result data(U&& t) noexcept(std::is_nothrow_convertible_v<U, T>) {
        return Result{std::forward<U>(t)};
    }

    static auto alwaysError() noexcept {
        return make([](Stream& s) {
           return makeError("Always error", s.pos());
        });
    }
private:
    StoredFn m_fn;
};

}

// #include <parsecpp/core/lazy.h>


// #include <parsecpp/core/parser.h>


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

    explicit LazyCached(Fn const& generator) noexcept {
        if (!insideWork) { // cannot use optional because generator() invoke ctor before optional::emplace
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


template <auto = details::SourceLocation::current().hash()>
struct AutoTag;

template <typename Tag = AutoTag<>, std::invocable Fn>
auto lazyCached(Fn const& genParser) noexcept(std::is_nothrow_invocable_v<Fn>) {
    return make_parser(LazyCached<Tag, Fn>{genParser});
}


template <typename T, typename tag = AutoTag<>>
struct LazyForget {
    using InvokerType = typename Parser<T>::Result (*)(void*, Stream&);

    template <typename ParserOut>
    InvokerType makeInvoker() noexcept {
        return [](void* pParser, Stream& s) {
            return static_cast<ParserOut*>(pParser)->apply(s);
        };
    }
public:
    template <typename Fn>
        requires(!std::is_same_v<Fn, LazyForget<T>>)
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


template <typename T, typename Fn, typename Tag = AutoTag<>>
auto lazyForget(Fn const& f) noexcept {
    return Parser<T>::make(LazyForget<T, Tag>{f});
}

}
// #include <parsecpp/core/lift.h>


// #include <parsecpp/core/parser.h>

// #include <parsecpp/common/base.h>


// #include <parsecpp/core/parser.h>

// #include <parsecpp/utils/cmp.h>


#include <concepts>
#include <utility>


namespace prs::details {

template <typename T, typename First, typename ...Args>
bool cmpAnyOf(T const& t, First &&f, Args &&...args) noexcept {
    if constexpr (sizeof...(args) > 0) {
        return f == t || cmpAnyOf(t, std::forward<Args>(args)...);
    } else {
        return f == t;
    }
}

}

namespace prs {


template <typename T>
auto pure(T tValue) noexcept {
    using Value = std::decay_t<T>;
    return Parser<Value>::make([t = std::move(tValue)](Stream&) {
        return Parser<Value>::data(t);
    });
}


inline auto success() noexcept {
    return pure(Unit{});
}


inline auto fail() noexcept {
    return Parser<Unit>::make([](Stream& s) {
        return Parser<Unit>::makeError("Fail", s.pos());
    });
}


inline auto fail(std::string const& text) noexcept {
    return Parser<Unit>::make([text](Stream& s) {
        return Parser<Unit>::makeError(text, s.pos());
    });
}

}

namespace prs {


template <typename Fn>
auto make_parser(Fn &&f) noexcept {
    return Parser<typename std::invoke_result_t<Fn, Stream&>::Body>::make(std::forward<Fn>(f));
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

}

template <typename Fn, typename ...Args>
auto liftM(Fn fn, Args &&...args) noexcept {
    return make_parser([fn, parsers = std::make_tuple(args...)](Stream& s) {
        return details::liftRec(fn, s, parsers);
    });
}

template <typename ...Args>
auto concat(Args &&...args) noexcept {
    return liftM(details::MakeTuple{}, std::forward<Args>(args)...);
}

template <typename Fn>
auto satisfy(Fn&& tTest) noexcept {
    return Parser<char>::make([test = std::forward<Fn>(tTest)](Stream& stream) {
        if (auto c = stream.checkFirst(test); c != 0) {
            return Parser<char>::data(c);
        } else {
            return Parser<char>::PRS_MAKE_ERROR("satisfy", stream.pos());
        }
    });
}

}
// #include <parsecpp/common/base.h>

// #include <parsecpp/common/number.h>


// #include <parsecpp/core/parser.h>


#include <concepts>
#include <charconv>


namespace prs {

template <typename Number = double>
    requires (std::is_arithmetic_v<Number>)
auto number() noexcept {
    return Parser<Number>::make([](Stream& s) {
        auto sv = s.sv();
        Number n{};
        if (auto res = std::from_chars(sv.data(), sv.data() + sv.size(), n);
                res.ec == std::errc{}) {
            s.moveUnsafe(res.ptr - sv.data());
            return Parser<Number>::data(n);
        } else {
            // TODO: get error and pos from res
            return Parser<Number>::makeError("Cannot parse number", s.pos());
        }
    });
}

}
// #include <parsecpp/common/string.h>


// #include <parsecpp/core/parser.h>

// #include <parsecpp/core/lift.h>

// #include <parsecpp/utils/funcHelper.h>


namespace prs {

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

    return prs::make_parser(p);
}

inline auto spaces() noexcept {
    return make_parser([](Stream& str) {
        while (str.checkFirst([](char c) {
            return std::isspace(c);
        }));

        return prs::Parser<Unit>::data({});
    });
}


inline auto spacesFast() noexcept {
    return make_parser([](Stream& str) {
        while (str.checkFirst([](char c) {
            return c == ' ';
        }));

        return prs::Parser<Unit>::data({});
    });
}

template <bool allowEmpty = true, bool allowDigit = false, typename StringType = std::string_view>
auto letters() noexcept {
    return make_parser([](Stream& str) {
        auto start = str.pos();
        while (str.checkFirst([](char c) {
            return ('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z') || (allowDigit && ('0' <= c && c <= '9'));
        }));

        auto end = str.pos();
        if (!allowEmpty && start == end) {
            return Parser<StringType>::makeError("Empty word", str.pos());
        } else {
            return Parser<StringType>::data(StringType{str.get_sv(start, end)});
        }
    });
}

class FromRange {
public:
    FromRange(char begin, char end) noexcept
        : m_begin(begin)
        , m_end(end) {

    }

    friend bool operator==(FromRange const& range, char c) noexcept {
        return range.m_begin <= c && c <= range.m_end;
    }
private:
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

template <LeftCmpWith<char> ...Args>
auto charFrom(Args ...chars) noexcept {
    return satisfy([=](char c) {
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


template <ParserType P>
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

}
// #include <parsecpp/common/map.h>


// #include <parsecpp/core/parser.h>

// #include <parsecpp/core/lift.h>



#include <map>


namespace prs {

template <bool errorInTheMiddle = true
        , size_t maxIteration = MAX_ITERATION
        , ParserType ParserKey
        , ParserType ParserValue>
auto toMap(ParserKey key, ParserValue value) noexcept {
    using Key = parser_result_t<ParserKey>;
    using Value = parser_result_t<ParserValue>;
    using Map = std::map<Key, Value>;
    using P = Parser<Map>;
    return P::make([key, value](Stream& stream) {
        Map out{};
        size_t iteration = 0;

        [[maybe_unused]]
        auto backup = stream.pos();
        do {
            auto keyRes = key.apply(stream);
            if (!keyRes.isError()) {
                auto valueRes = value.apply(stream);
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


template <bool errorInTheMiddle = true
        , size_t maxIteration = MAX_ITERATION
        , ParserType ParserKey
        , ParserType ParserValue
        , ParserType ParserDelimiter>
auto toMap(ParserKey tKey, ParserValue tValue, ParserDelimiter tDelimiter) noexcept {
    using Key = parser_result_t<ParserKey>;
    using Value = parser_result_t<ParserValue>;
    using Map = std::map<Key, Value>;
    using P = Parser<Map>;
    return P::make([key = std::move(tKey), value = std::move(tValue), delimiter = std::move(tDelimiter)](Stream& stream) {
        Map out{};
        size_t iteration = 0;

        auto backup = stream.pos();
        do {
            auto keyRes = key.apply(stream);
            if (!keyRes.isError()) {
                auto valueRes = value.apply(stream);
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
        } while (!delimiter.apply(stream).isError() && ++iteration != maxIteration);

        if (iteration == maxIteration) {
            return P::makeError("Max iteration", stream.pos());
        } else {
            stream.restorePos(backup);
            return P::data(std::move(out));
        }
    });
}

}


// #include <parsecpp/utils/applyFirstMatch.h>


#include <functional>
#include <tuple>

namespace prs::details {

template <typename UnhandledAction, typename Equal, typename ...TupleArgs>
class ApplyFirstMatch {
public:
    explicit constexpr ApplyFirstMatch(UnhandledAction unhandled, Equal eq, TupleArgs &&...args) noexcept
        : m_tuple(std::make_tuple(std::forward<TupleArgs>(args)...))
        , m_unhandled(std::move(unhandled))
        , m_cmp(std::move(eq)) {
    }

    template <typename KeyLike, typename ...Args>
    constexpr decltype(auto) apply(KeyLike const& key, Args &&...args) const {
        auto f = [&](const auto& el) -> bool {
            return std::invoke(m_cmp, el, key);
        };

        using ResultType = decltype(invoke(m_unhandled, args...));
        return foreach<ResultType, 0>(f, std::forward<Args>(args)...);
    }

    static constexpr size_t size() noexcept {
        return sizeof ...(TupleArgs);
    }
private:
    template <typename F, typename ...Args>
    constexpr decltype(auto) invoke(F f, Args &&...args) const {
        if constexpr (std::is_invocable_v<F, Args...>) {
            return std::invoke(f, std::forward<Args>(args)...);
        } else {
            static_assert(sizeof...(Args) == 0);
            return f;
        }
    }

    template <typename ResultType, unsigned shift, typename ...Args>
    constexpr ResultType foreach(auto check_fn, Args &&...args) const {
        const auto& [key, fn] = get<shift>(m_tuple);
        if (check_fn(key)) {
            return invoke(fn, std::forward<Args>(args)...);
        } else {
            if constexpr (std::tuple_size_v<decltype(m_tuple)> > shift + 1) {
                return foreach<ResultType, shift + 1>(check_fn, args...);
            } else {
                return invoke(m_unhandled, std::forward<Args>(args)...);
            }
        }
    }

    const std::tuple<TupleArgs...> m_tuple;
    const UnhandledAction m_unhandled;
    const Equal m_cmp;
};


template <typename UnhandledAction, typename ...TupleArgs>
static constexpr auto makeFirstMatch(UnhandledAction unhandled, TupleArgs &&...args) noexcept {
    return ApplyFirstMatch(unhandled, std::equal_to<>{}, std::forward<TupleArgs>(args)...);
}

template <typename UnhandledAction, typename Equal, typename ...TupleArgs>
static constexpr auto makeFirstMatchEq(UnhandledAction unhandled, Equal eq, TupleArgs &&...args) noexcept {
    return ApplyFirstMatch(unhandled, eq, std::forward<TupleArgs>(args)...);
}

}


// #include <parsecpp/common/debug.h>


// #include <parsecpp/core/parser.h>

// #include <parsecpp/utils/cmp.h>


namespace prs::debug {

class DebugParser;

//template <bool enable = true>
class DebugEnvironment {
public:
    struct PrintEOSHelper {
    public:
        static constexpr char EOS_SYMBOL = '\0';

        PrintEOSHelper(std::string_view full, size_t pos) noexcept {
            if (full.size() > pos) {
                m_symbol = full[pos];
            } else {
                m_symbol = EOS_SYMBOL;
            }
        }

        friend std::ostream& operator<<(std::ostream& os, PrintEOSHelper helper) noexcept {
            if (helper.m_symbol == EOS_SYMBOL) {
                os << "_EOS_";
            } else {
                os << helper.m_symbol;
            }
            return os;
        }

    private:
        char m_symbol = EOS_SYMBOL;
    };

    DebugEnvironment() noexcept = default;

    std::string print(Stream const& stream) const noexcept {
        std::stringstream out;
        out << "Parse '" << stream.full() << "'\n";
        auto step = 0u;
        for (auto const& callInfo : m_callStack) {
            out << step++ << ". pos: " << callInfo.pos
                << "(" << PrintEOSHelper(stream.full(), callInfo.pos) << ")"
                << " desc: " << callInfo.desc << "\n";
        }

        return out.str();
    }

    void addLog(size_t pos, std::string desc) const noexcept {
        m_callStack.emplace_back(pos, std::move(desc));
    }

    void clear() noexcept {
        m_callStack.clear();
    }

    struct CallInfo {
        CallInfo(size_t t_pos, std::string t_desc) noexcept
            : pos(t_pos)
            , desc(std::move(t_desc)) {

        }

        size_t pos = std::string_view::npos;
        std::string desc;
    };

private:
    mutable std::vector<CallInfo> m_callStack;
};


class DebugParser {
public:
    explicit DebugParser(DebugEnvironment& env) noexcept
        : m_env(env) {

    }

    void addLog(size_t pos, std::string desc) const noexcept {
        m_env.addLog(pos, std::move(desc));
    }

    using CallInfo = DebugEnvironment::CallInfo;
private:
    DebugEnvironment& m_env;
};

struct LogPoint : private DebugParser {
public:
    using P = Parser<Drop>;

    LogPoint(DebugEnvironment& env, std::string const& name) noexcept
        : DebugParser(env)
        , m_desc("Point{" + name + "}") {

    }

    P::Result operator()(Stream& stream) const noexcept {
        addLog(stream.pos(), m_desc);
        return P::data({});
    }
private:
    std::string m_desc;
};


inline auto logPoint(DebugEnvironment& env, std::string name) noexcept {
    return make_parser(LogPoint{env, std::move(name)});
}

template <bool onlyError, ParserType ParserA>
struct ParserWork : private DebugParser {
public:
    using Result = typename ParserA::Result;
    ParserWork(DebugEnvironment& env, ParserA tParser, std::string parserName) noexcept
        : DebugParser(env)
        , m_parser(std::move(tParser))
        , m_parserName(std::move(parserName)) {

    }

    Result operator()(Stream& stream) const noexcept(ParserA::nothrow) {
        if constexpr (!onlyError) {
            addLog(stream.pos(), "Before{" + m_parserName + "}");
        }

        return m_parser.apply(stream).join([&](auto&& t) {
            if constexpr (!onlyError) {
                addLog(stream.pos(), "After{" + m_parserName + "}");
            }
            return Result{t};
        }, [&](auto &&error) {
            addLog(stream.pos(), "Error{" + m_parserName + "}: " + error.description);
            return Result{error};
        });
    }
private:
    ParserA m_parser;
    std::string m_parserName;
};

template <ParserType ParserA>
auto parserWork(DebugEnvironment& env, ParserA tParser, std::string parserName) noexcept {
    return ParserA::make(ParserWork<false, std::decay_t<ParserA>>{env, std::move(tParser), std::move(parserName)});
}


template <ParserType ParserA>
auto parserError(DebugEnvironment& env, ParserA tParser, std::string parserName) noexcept {
    return ParserA::make(ParserWork<true, std::decay_t<ParserA>>{env, std::move(tParser), std::move(parserName)});
}


template <ParserType P>
class DebugExecutor {
public:
    template <typename Fn>
    DebugExecutor(std::unique_ptr<DebugEnvironment> spEnv, Fn&& fn) noexcept(std::is_nothrow_invocable_v<Fn, DebugEnvironment&>) {

    }
private:
    std::unique_ptr<DebugEnvironment> m_spEnv;
    P m_parser;
};
//
//template <typename Fn>
//auto makeDebug(Fn&& f) noexcept(std::is_nothrow_invocable_v<Fn, DebugEnvironment&>) {
//    return []
//}
}