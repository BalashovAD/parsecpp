#pragma once

#include <parsecpp/core/parser.h>
#include <parsecpp/core/modifier.h>

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