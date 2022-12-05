#pragma once

#include <parsecpp/core/parser.h>

#include <concepts>
#include <charconv>


namespace prs {

template <typename Number = double>
requires (std::is_arithmetic_v<Number>)
auto number() noexcept {
    return Parser<Number>::make([](Stream& s) {
        auto sv = s.sv();
        Number n{};
        if (auto res = std::from_chars(sv.data(), sv.data() + sv.size(), n);
                res.ec == std::errc()) {
            s.move(res.ptr - sv.data());
            return Parser<Number>::data(n);
        } else {
            // TODO: get error and pos from res
            return Parser<Number>::error("Cannot parse number", s.pos());
        }
    });
}

}