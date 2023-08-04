#pragma once

namespace prs {

template<std::size_t N>
struct ConstexprString {
    std::array<char, N + 1> m_str;

    constexpr ConstexprString(char const(&s)[N + 1]) noexcept {
        for (std::size_t i = 0; i <= N; ++i) {
            m_str[i] = s[i];
        }
    }

    constexpr ConstexprString(std::array<char, N + 1> arr) noexcept
        : m_str(arr) {
    }

    static constexpr ConstexprString<1> fromChar(char c) noexcept {
        return ConstexprString<1>({c});
    }

    constexpr const char* c_str() const noexcept {
        return m_str.data();
    }

    constexpr size_t size() const noexcept {
        return N;
    }

    constexpr std::string_view sv() const noexcept {
        return std::string_view(c_str(), size());
    }

    std::string toString() const noexcept {
        return std::string(c_str(), size());
    }

    template<std::size_t M>
    constexpr auto operator+(ConstexprString<M> const& other) const noexcept {
        std::array<char, N + M + 1> new_str{};
        for (std::size_t i = 0; i != N; ++i) {
            new_str[i] = m_str[i];
        }
        for (std::size_t i = 0; i != M; ++i) {
            new_str[N + i] = other.m_str[i];
        }
        return ConstexprString<N + M>(new_str);
    }

    constexpr auto add(char c) const noexcept {
        return operator+(fromChar(c));
    }

    constexpr auto between(char c) const noexcept {
        return fromChar(c) + *this + fromChar(c);
    }
};

template <typename T, T... chars>
constexpr auto operator""_prs() {
    return ConstexprString<sizeof...(chars)>({chars...});
}


}