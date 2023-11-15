#pragma once

#include <parsecpp/core/buildParams.h>

#include <string>

namespace prs::details {

template <bool disableDescription>
struct ParsingErrorT;

template <>
struct ParsingErrorT<false> {
    ParsingErrorT() = default;

    explicit ParsingErrorT(std::string_view s, size_t p) noexcept
        : description(s), pos(p) {};

    explicit ParsingErrorT(size_t p) noexcept
        : pos(p) {};

    std::string_view getDescription() const noexcept {
        return description;
    }

    std::string description;
    size_t pos{};
};

template <>
struct ParsingErrorT<true> {
    ParsingErrorT() = default;

    explicit ParsingErrorT(std::string_view s, size_t p) noexcept
        : pos(p) {};

    explicit ParsingErrorT(size_t p) noexcept
        : pos(p) {};

    std::string_view getDescription() const noexcept {
        return "";
    }

    size_t pos{};
};

using ParsingError = ParsingErrorT<DISABLE_ERROR_LOG>;

}