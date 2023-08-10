#pragma once

namespace prs {

namespace details {

template<std::size_t N>
struct MakeArray
{
    std::array<char, N> data;

    template <std::size_t... Is>
    constexpr MakeArray(char const(&arr)[N], std::integer_sequence<std::size_t, Is...>)
        : data{arr[Is]...} {

    }

    constexpr MakeArray(char const(&arr)[N])
        : MakeArray(arr, std::make_integer_sequence<std::size_t, N>()) {

    }

    constexpr auto size() const {
        return N;
    }
};

}

template<std::size_t N>
struct ConstexprString {
    std::array<char, N + 1> m_str;

    constexpr explicit ConstexprString(char const(&s)[N + 1]) noexcept {
        for (std::size_t i = 0; i <= N; ++i) {
            m_str[i] = s[i];
        }
    }

    constexpr explicit ConstexprString(std::array<char, N + 1> arr) noexcept
        : m_str(arr) {
    }

    constexpr explicit ConstexprString(details::MakeArray<N + 1> arr) noexcept{
        for (size_t i = 0; i != N + 1; ++i) {
            m_str[i] = arr.data[i];
        }
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


    constexpr auto between(char l, char r) const noexcept {
        return fromChar(l) + *this + fromChar(r);
    }


    template <size_t M>
    constexpr bool operator==(ConstexprString<M> const& rhs) const noexcept {
        if constexpr (M != N) {
            return false;
        } else {
            for (auto i = 0; i != N; ++i) {
                if (rhs[i] != this->operator[](i)) {
                    return false;
                }
            }
            return true;
        }
    }

    template <size_t M>
    constexpr bool operator!=(ConstexprString<M> const& rhs) const noexcept {
        return !operator==(rhs);
    }

    constexpr auto operator[](size_t i) const noexcept {
        return m_str[i];
    }


    template <size_t M>
    constexpr ConstexprString<std::min(M, N)> substr() const noexcept {
        constexpr auto T = std::min(M, N);
        std::array<char, T + 1> out{};
        for (auto i = 0; i != T; ++i) {
            out[i] = operator[](i);
        }
        return ConstexprString<T>(out);
    }

    template <size_t M>
    requires(M <= N)
    constexpr bool startsFrom(ConstexprString<M> const& prefix) const noexcept {
        return substr<M>() == prefix;
    }
};

template <details::MakeArray arr>
constexpr auto operator""_prs() {
    return ConstexprString<arr.size() - 1>(arr);
}


}