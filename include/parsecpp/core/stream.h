#pragma once

#include <parsecpp/core/parsingError.h>
#include <parsecpp/core/buildParams.h>

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
        if constexpr (DISABLE_ERROR_LOG) {
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