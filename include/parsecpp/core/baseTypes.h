#pragma once

#include <parsecpp/core/concept.h>
#include <parsecpp/core/context.h>

namespace prs {


struct Unit {
    Unit() noexcept = default;

    bool operator==(Unit const&) const noexcept {
        return true;
    }
};


struct Drop {
    Drop() noexcept = default;

    bool operator==(Drop const&) const noexcept {
        return true;
    }
};

namespace details {

template <class Body>
using ResultType = Expected<Body, ParsingError>;

template <typename T, ContextType Ctx = VoidContext>
using StdFunction = std::conditional_t<IsVoidCtx<Ctx>,
                std::function<ResultType<T>(Stream&)>, std::function<ResultType<T>(Stream&, Ctx&)>>;

}



};
