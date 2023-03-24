#pragma once

#include <parsecpp/core/expected.h>
#include <parsecpp/utils/funcHelper.h>
#include <parsecpp/core/parsingError.h>
#include <parsecpp/core/context.h>
#include <parsecpp/core/stream.h>

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

template <typename T, ContextType Ctx = VoidContext>
using StdFunction = std::conditional_t<IsVoidCtx<Ctx>,
        std::function<ResultType<T>(Stream&)>, std::function<ResultType<T>(Stream&, Ctx&)>>;

}

template <typename T, ContextType CtxType = VoidContext, typename Func = details::StdFunction<T, CtxType>>
//        requires std::invocable<Func, Stream&>
class Parser {
public:
    static constexpr bool nothrow = std::is_nothrow_invocable_v<Func, Stream&>;
    static constexpr bool nocontext = IsVoidCtx<CtxType>;

    using StoredFn = std::decay_t<Func>;

    using Type = T;
    using Ctx = CtxType;
    using Result = details::ResultType<T>;

//    template <std::invocable<Stream&> Fn>
    template <typename Fn>
    constexpr static auto make(Fn &&f) noexcept {
        return Parser<T, Ctx, Fn>(std::forward<Fn>(f));
    }


    template <typename U>
        requires(std::is_same_v<std::decay_t<U>, StoredFn>)
    constexpr explicit Parser(U&& f) noexcept
        : m_fn(std::forward<U>(f)) {

    }

    constexpr Result operator()(Stream& stream) const noexcept(nothrow) {
        if constexpr (!std::is_invocable_v<StoredFn, Stream&> && std::is_same_v<Ctx, VoidContext>) {
            return std::invoke(m_fn, stream, VOID_CONTEXT);
        } else {
            return std::invoke(m_fn, stream);
        }
    }

    template <ContextType Context>
    constexpr Result operator()(Stream& stream, Context& ctx) const noexcept(nothrow) {
        if constexpr (std::is_invocable_v<StoredFn, Stream&, Context&>) {
            return std::invoke(m_fn, stream, ctx);
        } else {
            return std::invoke(m_fn, stream);
        }
    }


    constexpr Result apply(Stream& stream) const noexcept(nothrow) {
        return operator()(stream);
    }

    template <typename Context>
    constexpr Result apply(Stream& stream, Context& ctx) const noexcept(nothrow) {
        return operator()(stream, ctx);
    }

    /**
     * @def `>>` :: Parser<A> -> Parser<B> -> Parser<B>
     */
    template <typename B, typename CtxB, typename Rhs>
        requires (IsVoidCtx<UnionCtx<Ctx, CtxB>>)
    constexpr auto operator>>(Parser<B, CtxB, Rhs> rhs) const noexcept {
        return Parser<B, VoidContext>::make([lhs = *this, rhs](Stream& stream) noexcept(nothrow && Parser<B, Ctx, Rhs>::nothrow) {
            return lhs.apply(stream).flatMap([&rhs, &stream](T const& body) noexcept(Parser<B, Ctx, Rhs>::nothrow) {
                return rhs.apply(stream);
            });
        });
    }


    /**
     * @def `>>` :: Parser<A> -> Parser<B> -> Parser<B>
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
    constexpr auto operator<<(Parser<B, CtxB, Rhs> rhs) const noexcept {
        constexpr bool firstCallNoexcept = nothrow && Parser<B, Ctx, Rhs>::nothrow;
        if constexpr (IsVoidCtx<Ctx>) {
            return Parser<T>::make([lhs = *this, rhs](Stream& stream) noexcept(firstCallNoexcept) {
                return lhs.apply(stream).flatMap([&rhs, &stream](T&& body) noexcept(Parser<Rhs>::nothrow) {
                    return rhs.apply(stream).map([mvBody = std::move(body)](auto const& _) {
                        return mvBody;
                    });
                });
            });
        } else {
            return Parser<T>::make([lhs = *this, rhs](Stream& stream, auto& ctx) noexcept(firstCallNoexcept) {
                return lhs.apply(stream, ctx).flatMap([&rhs, &stream, &ctx](T body) noexcept(Parser<Rhs>::nothrow) {
                    return rhs.apply(stream, ctx).map([mvBody = std::move(body)](auto const& _) {
                        return mvBody;
                    });
                });
            });
        }
    }


    /*
     * <$>, fmap operator
     * @def `>>=` :: Parser<A> -> (A -> B) -> Parser<B>
     */
    template <std::invocable<T> ListFn>
    constexpr friend auto operator>>=(Parser lhs, ListFn fn) noexcept {
        return Parser<std::invoke_result_t<ListFn, T>>::make([lhs, fn](Stream& str) {
           return lhs.apply(str).map(fn);
        });
    }


    /**
     * @def `|` :: Parser<A> -> Parser<A> -> Parser<A>
     */
    template <typename Rhs>
    constexpr auto operator|(Parser<T, Ctx, Rhs> rhs) const noexcept {
        return Parser<T>::make([lhs = *this, rhs](Stream& stream, auto& ctx) {
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
     */
    constexpr auto maybe() const noexcept {
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


    /**
     * @def maybeOr :: Parser<A> -> Parser<A>
     */
    constexpr auto maybeOr(T const& defaultValue) const noexcept {
        return Parser<T>::make([parser = *this, defaultValue](Stream& stream) {
            auto backup = stream.pos();
            return parser.apply(stream).flatMapError([&stream, &backup, &defaultValue](details::ParsingError const& error) {
                stream.restorePos(backup);
                return data(defaultValue);
            });
        });
    }


    /**
     * @def repeat :: Parser<A> -> Parser<std::vector<A>>
     */
    template <size_t reserve = 0, size_t maxIteration = MAX_ITERATION>
            requires(!std::is_same_v<T, Drop>)
    constexpr auto repeat() const noexcept {
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

    /**
     * @def repeat :: Parser<Drop> -> Parser<Drop>
     */
    template <size_t maxIteration = MAX_ITERATION>
            requires(std::is_same_v<T, Drop>)
    constexpr auto repeat() const noexcept {
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


    /**
     * @def repeat :: Parser<A> -> Parser<Delim> -> Parser<std::vector<A>>
     */
    template <size_t reserve = 0, size_t maxIteration = MAX_ITERATION, ParserType Delimiter>
            requires(!std::is_same_v<T, Drop>)
    constexpr auto repeat(Delimiter tDelimiter) const noexcept {
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


    /**
     * @def repeat :: Parser<Drop> -> Parser<Delim> -> Parser<Drop>
     */
    template <size_t maxIteration = MAX_ITERATION, ParserType Delimiter>
            requires(std::is_same_v<T, Drop>)
    constexpr auto repeat(Delimiter tDelimiter) const noexcept {
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


    /**
     * @def cond :: Parser<A> -> Parser<A>
     */
    template <std::predicate<T const&> Fn>
            requires (!std::predicate<T const&, Stream&>)
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
     * @def cond :: Parser<A> -> Parser<A>
     */
    template <std::predicate<T const&, Stream&> Fn>
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
     * @def Drop :: Parser<A> -> Parser<Drop>
     */
    constexpr auto drop() const noexcept {
        return Parser<Drop>::make([p = *this](Stream& s) {
            return p.apply(s).map([](auto &&) {
                return Drop{};
            });
        });
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
    constexpr auto flatMap(Fn fn) const noexcept(std::is_nothrow_invocable_v<Fn, T const&>) {
        return make_parser([parser = *this, fn](Stream& stream) {
            return parser.apply(stream).flatMap([fn, &stream](T const& t) {
                return fn(t).apply(stream);
            });
        });
    }

    /**
     * @def toCommonType :: Parser<A, Fn> -> Parser'<A>
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

    static constexpr Result makeError(details::ParsingError error) noexcept {
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
