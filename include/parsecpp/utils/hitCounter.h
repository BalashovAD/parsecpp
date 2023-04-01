#pragma once

#include <parsecpp/core/parser.h>

namespace prs {

template <typename Tag = unsigned>
using HitCounterType = details::NamedType<unsigned, Tag>;


template <typename Tag = unsigned>
auto inline hitCounter() noexcept {
    using Ctx = ContextWrapper<HitCounterType<Tag>>;
    return Parser<Unit, Ctx>::make([](Stream& stream, Ctx& counter) noexcept {
        ++counter.get();
        return Parser<Unit, Ctx>::data({});
    });
}

}