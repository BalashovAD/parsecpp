#pragma once

#include <parsecpp/core/parser.h>
#include <parsecpp/core/lift.h>


#include <map>


namespace prs {

template <template <typename K, typename V> typename MapType = std::map,
        typename ParserKey,
        typename ParserValue>
auto toMap(ParserKey key, ParserValue value) noexcept {
    using Key = parser_result_t<ParserKey>;
    using Value = parser_result_t<ParserValue>;
    using Map = MapType<Key, Value>;
    using P = Parser<Map>;
    return P::make([key, value](Stream& stream) {
        Map ans{};

        auto backup = stream.pos();
        const auto p = [&](auto const& rec) -> void {
            liftM([&](auto const& key, auto const& value) {
                return ans.insert_or_assign(key, value).second;
            }, key, value).apply(stream).map([&](bool wasAdded) {
                backup = stream.pos();
                rec();
                return wasAdded;
            });
        };

        details::Y{p}();
        stream.restorePos(backup);
        return P::data(ans);
    });
}


template <template <typename K, typename V> typename MapType = std::map,
        typename ParserKey,
        typename ParserValue,
        typename ParserDelimiter>
auto toMap(ParserKey key, ParserValue value, ParserDelimiter delimiter) noexcept {
    using Key = parser_result_t<ParserKey>;
    using Value = parser_result_t<ParserValue>;
    using Map = MapType<Key, Value>;
    using P = Parser<Map>;
    return P::make([key, value, delimiter](Stream& stream) {
        Map ans{};

        auto backup = stream.pos();
        const auto p = [&](auto const& rec, bool needDelimiter) -> void {
            if (needDelimiter) {
                (delimiter >> liftM([&](auto const& key, auto const& value) {
                    return ans.insert_or_assign(key, value).second;
                }, key, value)).apply(stream).map([&](bool wasAdded) {
                    backup = stream.pos();
                    rec(true);
                    return wasAdded;
                });
            } else {
                liftM([&](auto const& key, auto const& value) {
                    return ans.insert_or_assign(key, value).second;
                }, key, value).apply(stream).map([&](bool wasAdded) {
                    backup = stream.pos();
                    rec(true);
                    return wasAdded;
                });
            }
        };

        details::Y{p}(false);
        stream.restorePos(backup);
        return P::data(ans);
    });
}

template <typename ParserValue, typename ParserDelimiter>
auto toArray(ParserValue value, ParserDelimiter delimiter) noexcept {
    using Value = parser_result_t<ParserValue>;
    using P = Parser<std::vector<Value>>;
    return P::make([value, delimiter](Stream& stream) {
        std::vector<Value> ans{};

        auto backup = stream.pos();

        const auto emplace = [&](auto const& value) {
            ans.emplace_back(value);
            return Unit{};
        };

        const auto p = [&](auto const& rec, bool needDelimiter) -> void {
            if (needDelimiter) {
                (delimiter >> value >>= emplace).apply(stream).map([&](Unit const& _) {
                    backup = stream.pos();
                    rec(true);
                    return _;
                });
            } else {
                (value >>= emplace).apply(stream).map([&](Unit const& _) {
                    backup = stream.pos();
                    rec(true);
                    return _;
                });
            }
        };

        details::Y{p}(false);
        stream.restorePos(backup);
        return P::data(ans);
    });
}

}