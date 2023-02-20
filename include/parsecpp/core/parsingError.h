#pragma once

#include <parsecpp/core/buildParams.h>
#include <parsecpp/utils/sourceLocation.h>

#include <string>

namespace prs::details {

template <bool disable>
struct ParsingDebugInfo;

template <>
struct ParsingDebugInfo<true> {};

template <>
struct ParsingDebugInfo<false> {
    std::string description = {};
    SourceLocation location;
};

struct ParsingError : ParsingDebugInfo<DISABLE_ERROR_LOG> {
    size_t pos{};
};


}