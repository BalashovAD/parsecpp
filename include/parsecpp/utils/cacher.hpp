#pragma once

namespace prs {

namespace details {




}

template <typename Out, typename Key, typename Fn, template <typename K, typename V> typename StorageT = std::map>
class StaticCacher {
public:
    template <typename K, typename V>
    using Storage = StorageT<K, V>;

    explicit StaticCacher(Fn fn)
        : m_fn(std::move(fn)) {

    }


    template <typename ...Args>
        requires(sizeof...(Args) > 1)
    Out const& operator()(Args const& ...args) const {
        auto key = std::make_tuple(args...);
        if (auto it = m_storage.find(key); it != m_storage.end()) {
            return it->second;
        } else {
            return m_storage.emplace(key, m_fn(args...)).first->second;
        }
    }


    Out const& operator()(Key const& key) const {
        if (auto it = m_storage.find(key); it != m_storage.end()) {
            return it->second;
        } else {
            return m_storage.emplace(key, m_fn(key)).first->second;
        }
    }
private:
    mutable Storage<Key, Out> m_storage;
    Fn m_fn;
};

namespace details {

template <typename A, typename ...Args>
struct GetFirst {
    using type = A;
};

template <typename ...Args>
using getFirst = GetFirst<Args...>::type;

/*
 * template <typename A, typename ...>
 * using getFirst = A;
 * Compile error: pack expansion argument for non-pack parameter
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=59498
 */

}


template <typename ...Args, typename Fn>
auto staticCacher(Fn fn) {
    using Out = std::invoke_result_t<Fn, Args...>;
    using Key = std::conditional_t<sizeof...(Args) == 1, details::getFirst<Args...>, std::tuple<Args...>>;
    return StaticCacher<Out, Key, std::remove_cv_t<Fn>>{std::move(fn)};
}

}