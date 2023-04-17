#pragma once

#include <parsecpp/core/parser.h>
#include <parsecpp/core/forward.h>

namespace prs {


template <typename T, typename Ctx>
template <typename ParserType>
Pimpl<T, Ctx>::Pimpl(ParserType t) noexcept {
    static_assert(std::is_same_v<std::decay_t<ParserType>, Parser<T, Ctx>>, "Pimpl can store only Common parser type");
    static_assert(sizeof(t) == sizeof(m_storage));
    static_assert(alignof(ParserType) == alignof(std::function<void(void)>));
    new (&m_storage) ParserType(t);
}


template <typename T>
template <typename ParserType>
Pimpl<T, VoidContext>::Pimpl(ParserType t) noexcept {
    static_assert(std::is_same_v<std::decay_t<ParserType>, Parser<T>>, "Pimpl can store only Common parser type");
    static_assert(sizeof(t) == sizeof(m_storage));
    static_assert(alignof(ParserType) == alignof(std::function<void(void)>));
    new (&m_storage)ParserType(t);
}


template <typename T, typename Ctx>
Pimpl<T, Ctx>::~Pimpl() {
    using P = Parser<T, Ctx>;
    std::destroy_at(std::launder(reinterpret_cast<P*>(&m_storage)));
}


template <typename T>
Pimpl<T>::~Pimpl() {
    using P = Parser<T>;
    std::destroy_at(std::launder(reinterpret_cast<P*>(&m_storage)));
}


template<typename T, typename Ctx>
Expected<T, details::ParsingError> Pimpl<T, Ctx>::operator()(Stream& s, Ctx& ctx) const {
    using P = Parser<T, Ctx>;
    return reinterpret_cast<P const&>(m_storage).apply(s, ctx);
}


template<typename T>
Expected<T, details::ParsingError> Pimpl<T, VoidContext>::operator()(Stream& s) const {
    using P = Parser<T>;
    return reinterpret_cast<P const&>(m_storage).apply(s);
}

}