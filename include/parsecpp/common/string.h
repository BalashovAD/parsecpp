#pragma once

#include <parsecpp/core/parser.h>
#include <parsecpp/core/lift.h>
#include <parsecpp/utils/funcHelper.h>

namespace prs {

inline auto anyChar() noexcept {
    auto p = [](Stream& stream) {
        if (stream.eos()) {
            return prs::Parser<char>::makeError("empty string", stream.pos());
        } else {
            char c = stream.front();
            stream.move();
            return prs::Parser<char>::data(c);
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


template <bool forwardSearch = false>
auto searchText(std::string const& searchPattern) noexcept {
    return Parser<Unit>::make([searchPattern](Stream& stream) {
        auto str = stream.sv();
        if (auto pos = str.find(searchPattern); pos != std::string_view::npos) {
            if constexpr (forwardSearch) {
                stream.move(pos > 0 ? pos - 1 : 0);
                return Parser<Unit>::data({});
            } else {
                stream.move(pos + searchPattern.size());
                return Parser<Unit>::data({});
            }
        } else {
            return Parser<Unit>::PRS_MAKE_ERROR("Cannot find '" + std::string(searchPattern) + "'", stream.pos());
        }
    });
}

template <std::same_as<char> ...Args>
auto charIn(Args ...chars) noexcept {
    return satisfy([=](char c) {
        return details::cmpAnyOf(c, chars...);
    });
}


template <std::same_as<char> ...Args>
auto charInSpaces(Args ...chars) noexcept {
    return spaces() >> charIn(std::forward<Args>(chars)...) << spaces();
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


template <IsParser Parser>
auto between(char borderLeft, char borderRight, Parser parser) noexcept {
    return charIn(borderLeft) >> parser << charIn(borderRight);
}


template <IsParser Parser>
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
    }) ;
}

}