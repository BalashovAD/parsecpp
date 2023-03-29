#pragma once

#include <parsecpp/core/parser.h>
#include <parsecpp/core/lift.h>


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
    using Key = parser_result_t<ParserKey>;
    using Value = parser_result_t<ParserValue>;
    using Map = std::map<Key, Value>;
    using UCtx = UnionCtx<parser_ctx_t<ParserKey>, parser_ctx_t<ParserValue>>;
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
    using Key = parser_result_t<ParserKey>;
    using Value = parser_result_t<ParserValue>;
    using Map = std::map<Key, Value>;
    using UCtx = UnionCtx<parser_ctx_t<ParserKey>, parser_ctx_t<ParserValue>, parser_ctx_t<ParserDelimiter>>;
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