#pragma once

#include "parsecpp/utils/funcHelper.h"

#include <utility>
#include <cassert>


namespace prs {

template <typename T, typename Error>
    requires(!std::same_as<std::decay_t<T>, std::decay_t<Error>>)
class Expected {
public:
    using Body = T;

    template<typename OnSuccess, typename OnError = details::Id>
    static constexpr bool map_nothrow = std::is_nothrow_invocable_v<OnSuccess, T const&>
                    && std::is_nothrow_invocable_v<OnError, Error const&>;

    template<typename OnSuccess, typename OnError = details::Id>
    static constexpr bool map_move_nothrow = std::is_nothrow_invocable_v<OnSuccess, T>
                    && std::is_nothrow_invocable_v<OnError, Error>;

    constexpr explicit Expected(T &&t) noexcept
        : m_isError(false)
        , m_data(std::move(t)) {

    }


    constexpr explicit Expected(T const& t) noexcept
        : m_isError(false)
        , m_data(t) {

    }

    constexpr explicit Expected(Error &&error) noexcept
        : m_isError(true)
        , m_error(std::move(error)) {
    }

    constexpr explicit Expected(Error const& error) noexcept
        : m_isError(true)
        , m_error(error) {
    }

    constexpr Expected(Expected<T, Error> const& e) noexcept
        : m_isError(e.m_isError) {

        if (isError()) {
            new (&m_error)T(e.m_error);
        } else {
            new (&m_data)Error(e.m_data);
        }
    }

    ~Expected() noexcept {
        if (isError()) {
            m_error.~Error();
        } else {
            m_data.~T();
        }
    }

    constexpr bool isError() const noexcept {
        return m_isError;
    }

    T const& data() const& noexcept {
        assert(!isError());
        return m_data;
    }

    T data() && noexcept {
        assert(!isError());
        return std::move(m_data);
    }

    Error const& error() const& noexcept {
        assert(isError());
        return m_error;
    }

    Error error() && noexcept {
        assert(isError());
        return std::move(m_error);
    }


    template<std::invocable<T const&> OnSuccess, std::invocable<Error const&> OnError>
    auto join(OnSuccess onSuccess, OnError onError) const&
                    noexcept(map_nothrow<OnSuccess, OnError>) {

//        using Result = std::common_type_t<std::invoke_result_t<OnSuccess, const T&>,
//                    std::invoke_result_t<OnError, const Error&>>;

        return isError() ? onError(m_error) : onSuccess(m_data);
    }


    template<std::invocable<T> OnSuccess, std::invocable<Error> OnError>
    auto join(OnSuccess onSuccess, OnError onError) &&
                    noexcept(map_nothrow<OnSuccess, OnError>) {

//        using Result = std::common_type_t<std::invoke_result_t<OnSuccess, const T&>,
//                    std::invoke_result_t<OnError, const Error&>>;

        return isError() ? onError(std::move(m_error)) : onSuccess(std::move(m_data));
    }

    template<std::invocable<const T&> OnSuccess>
    auto map(OnSuccess onSuccess) const&
                    noexcept(map_nothrow<OnSuccess>) {

        using Result = std::invoke_result_t<OnSuccess, const T&>;
        return isError() ? Expected<Result, Error>{m_error} : Expected<Result, Error>{onSuccess(m_data)};
    }


    template<std::invocable<T> OnSuccess>
    auto map(OnSuccess onSuccess) &&
                    noexcept(map_move_nothrow<OnSuccess>) {

        using Result = std::invoke_result_t<OnSuccess, T&&>;
        return isError() ? Expected<Result, Error>{std::move(m_error)}
                : Expected<Result, Error>{onSuccess(std::move(m_data))};
    }

    template<std::invocable<const T&> OnSuccess, std::invocable<Error const&> OnError>
    auto map(OnSuccess onSuccess, OnError onError) const& noexcept(map_nothrow<OnSuccess>) {

        using Result = std::invoke_result_t<OnSuccess, T const&>;
        return isError() ? Expected<Result, Error>{onError(m_error)}
                : Expected<Result, Error>{onSuccess(m_data)};
    }

    template<std::invocable<T> OnSuccess, std::invocable<Error> OnError>
    auto map(OnSuccess onSuccess, OnError onError) && noexcept(map_move_nothrow<OnSuccess>) {

        using Result = std::invoke_result_t<OnSuccess, T>;
        return isError() ? Expected<Result, Error>{onError(std::move(m_error))}
                : Expected<Result, Error>{onSuccess(std::move(m_data))};
    }

    template<std::invocable<T const&> OnSuccess>
    auto flatMap(OnSuccess onSuccess) const& noexcept(map_nothrow<OnSuccess>) {

        using Result = std::invoke_result_t<OnSuccess, T const&>;
        return isError() ? Result{m_error} : onSuccess(m_data);
    }

    template<std::invocable<T&&> OnSuccess>
    auto flatMap(OnSuccess onSuccess) && noexcept(map_nothrow<OnSuccess>) {

        using Result = std::invoke_result_t<OnSuccess, T>;
        return isError() ? Result{std::move(m_error)} : onSuccess(std::move(m_data));
    }


    template<std::invocable<T const&> OnSuccess, std::invocable<Error const&> OnError>
    auto flatMap(OnSuccess onSuccess, OnError onError) const& noexcept(map_nothrow<OnSuccess, OnError>) {

        return isError() ? onError(m_error) : onSuccess(m_data);
    }


    template<std::invocable<T const&> OnSuccess, std::invocable<Error const&> OnError>
    auto flatMap(OnSuccess onSuccess, OnError onError) && noexcept(map_nothrow<OnSuccess, OnError>) {

        return isError() ? onError(std::move(m_error)) : onSuccess(std::move(m_data));
    }

    template<std::invocable<Error const&> OnError>
    auto flatMapError(OnError onError) const& noexcept(map_nothrow<details::Id, OnError>) {
        using Result = std::invoke_result_t<OnError, Error const&>;
        return isError() ? onError(m_error) : Result{m_data};
    }

    template<std::invocable<Error> OnError>
    auto flatMapError(OnError onError) && noexcept(map_nothrow<details::Id, OnError>) {
        using Result = std::invoke_result_t<OnError, Error>;
        return isError() ? onError(std::move(m_error)) : Result{std::move(m_data)};
    }
private:
    bool m_isError;

    union {
        T m_data;
        Error m_error;
    };
};


}