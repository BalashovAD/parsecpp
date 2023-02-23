#pragma once

#include <parsecpp/core/buildParams.h>
#include <parsecpp/utils/sourceLocation.h>

#include <string>

namespace prs::details {

struct ParsingError {
    std::string description;
    size_t pos{};
};


}