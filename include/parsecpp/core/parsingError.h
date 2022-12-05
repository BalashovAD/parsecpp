#pragma once

#include <string>

namespace prs::details {


struct ParsingError {
    std::string description;
    size_t pos;
};


}