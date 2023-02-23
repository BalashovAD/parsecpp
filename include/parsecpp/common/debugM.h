#pragma once

#include <parsecpp/core/parser.h>
#include <parsecpp/utils/cmp.h>

namespace prs::debug {

template <typename T>
class Writer {
private:
    T m_body;
    std::string m_msg;
};

}