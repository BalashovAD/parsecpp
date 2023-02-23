#pragma once

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
