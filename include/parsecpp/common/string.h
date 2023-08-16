#pragma once

#include <parsecpp/core/parser.h>
#include <parsecpp/core/lift.h>

#include <parsecpp/utils/constexprString.hpp>

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


struct AnySpace {
    constexpr AnySpace() = default;

    friend constexpr bool operator==(AnySpace const& range, char c) noexcept {
        return std::isspace(c);
    }
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


inline auto nextLine() noexcept {
    return Parser<Unit>::make([](Stream& stream) {
        auto sv = stream.sv();
        for (size_t i = 0; i != sv.size(); ++i) {
            if (sv[i] == '\n' || sv[i] == '\r') {
                if (sv[i] == '\r' && (i + 1) < sv.size() && sv[i + 1] == '\n') {
                    ++i;
                }
                stream.moveUnsafe(i);
                return Parser<Unit>::data({});
            }
        }
        stream.sv() = "";
        return Parser<Unit>::data({}); // return empty view if no new line found
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
        if (s.sv().starts_with(str.sv())) {
            s.move(str.size());
            return Parser<StringType>::data(str.sv());
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