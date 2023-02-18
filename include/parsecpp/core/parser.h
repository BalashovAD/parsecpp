#pragma once

#include "expected.h"
#include "parsecpp/utils/funcHelper.h"
#include "parsingError.h"
#include "stream.h"

#include <concepts>
#include <string_view>
#include <functional>
#include <utility>
#include <optional>


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

    using Type = T;
    using Result = details::ResultType<T>;

    template <std::invocable<Stream&> Fn>
    static auto make(Fn f) noexcept {
        return Parser<T, Fn>(f);
    }

    explicit Parser(Func f) noexcept
        : m_fn(std::move(f)) {

    }

    Result operator()(Stream& stream) const noexcept(nothrow) {
        return std::invoke(m_fn, stream);
    }


    Result apply(Stream& str) const noexcept(nothrow) {
        return operator()(str);
    }

    template <typename B, typename Rhs>
    auto operator>>(Parser<B, Rhs> rhs) const noexcept {
        return Parser<B>::make([lhs = *this, rhs](Stream& str) noexcept(nothrow && Parser<B, Rhs>::nothrow) {
            return lhs.apply(str).flatMap([&rhs, &str](T const& body) noexcept(Parser<B, Rhs>::nothrow) {
                return rhs.apply(str);
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
    using MaybeValue = std::conditional_t<std::is_same_v<A, Drop>, A, std::optional<A>>;

    auto maybe() const noexcept {
        return Parser<MaybeValue<T>>::make([parser = *this](Stream& stream) {
            auto backup = stream.pos();
            return parser.apply(stream).map([](T t) {
                return MaybeValue<T>{std::move(t)};
            }, [&stream, &backup](details::ParsingError const& error) {
                stream.restorePos(backup);
                return MaybeValue<T>{};
            });
        });
    }


    template <size_t reserve = 0, size_t maxIteration = MAX_ITERATION>
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

    template <size_t reserve = 0, size_t maxIteration = MAX_ITERATION, ParserType Delimiter>
    auto repeat(Delimiter tDelimiter) const noexcept {
        using Value = T;
        using P = Parser<std::vector<Value>>;
        return P::make([value = *this, delimiter = std::move(tDelimiter)](Stream& stream) {
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


    auto endOfStream() const noexcept {
        return cond([](T const& t, Stream& s) {
            return s.eos();
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
        return Result{details::ParsingError{.pos = pos}};
    }
#else
    static Result makeError(std::string desc, size_t pos) noexcept {
        return Result{details::ParsingError{std::move(desc), pos}};
    }
#endif

    static Result data(T t) noexcept {
        return Result{std::move(t)};
    }

    static auto alwaysError() noexcept {
        return make([](Stream& s) {
           return makeError("Always error", s.pos());
        });
    }
private:
    Func m_fn;
};

}