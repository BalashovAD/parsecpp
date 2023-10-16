#pragma once

namespace prs {

namespace details {


template <typename K, typename V>
class MapStorage {
public:
    MapStorage() = default;

    using Pointer = V const*;
    Pointer find(K const& key) const noexcept {
        if (auto it = m_storage.find(key); it != m_storage.end()) {
            return std::addressof(it->second);
        } else {
            return nullptr;
        }
    }

    V const& emplace(K const& k, V &&v) {
        return m_storage.emplace(k, v).first->second;
    }
private:
    std::map<K, V> m_storage;
};

template <typename K, typename V>
class HashMapStorage {
public:
    HashMapStorage() = default;

    using Pointer = V*;
    Pointer find(K const& key) noexcept {
        if (auto it = m_storage.find(key); it != m_storage.end()) {
            return std::addressof(it->second);
        } else {
            return nullptr;
        }
    }

    V const& emplace(K const& k, V &&v) {
        return m_storage.emplace(k, v).first->second;
    }
private:
    std::unordered_map<K, V> m_storage;
};


template <typename K, typename V>
class VectorStorage {
public:
    static constexpr size_t EXPECTED_MAX_ELEMENTS = 10;

    VectorStorage() {
        m_storage.reserve(EXPECTED_MAX_ELEMENTS);
    }

    using Pointer = V const*;
    Pointer find(K const& key) const noexcept {
        for (auto const& st : m_storage) {
            if (st.first == key) {
                return std::addressof(st.second);
            }
        }
        return nullptr;
    }

    V const& emplace(K const& k, V &&v) {
        assert(!find(k));

        return m_storage.emplace_back(std::pair<K, V>(k, v)).second;
    }
private:
    std::vector<std::pair<K, V>> m_storage;
};


}

template <typename Out, typename Key, typename Fn, typename StorageT>
class StaticCacher {
public:
    explicit StaticCacher(Fn fn)
        : m_fn(std::move(fn)) {

    }

    template <typename ...Args>
        requires(sizeof...(Args) > 1)
    Out const& operator()(Args const& ...args) const {
        auto key = std::make_tuple(args...);
        if (auto it = m_storage.find(key); it) {
            return *it;
        } else {
            return m_storage.emplace(key, m_fn(args...));
        }
    }


    Out const& operator()(Key const& key) const {
        if (auto it = m_storage.find(key); it) {
            return *it;
        } else {
            return m_storage.emplace(key, m_fn(key));
        }
    }
private:
    mutable StorageT m_storage;
    Fn m_fn;
};

namespace details {

template <typename A, typename ...Args>
struct GetFirst {
    using type = A;
};

template <typename ...Args>
using getFirst = typename GetFirst<Args...>::type;

/*
 * template <typename A, typename ...>
 * using getFirst = A;
 * Compile error: pack expansion argument for non-pack parameter
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=59498
 */

}


template <typename ...Args, typename Fn>
auto makeMapCacher(Fn fn) {
    using Out = std::invoke_result_t<Fn, Args...>;
    using Key = std::conditional_t<sizeof...(Args) == 1, details::getFirst<Args...>, std::tuple<Args...>>;
    return StaticCacher<Out, Key, std::remove_cv_t<Fn>, details::MapStorage<Key, Out>>{std::move(fn)};
}


template <template<typename K, typename V> typename Storage, typename ...Args, typename Fn>
auto makeTCacher(Fn fn) {
    using Out = std::invoke_result_t<Fn, Args...>;
    using Key = std::conditional_t<sizeof...(Args) == 1, details::getFirst<Args...>, std::tuple<Args...>>;
    return StaticCacher<Out, Key, std::remove_cv_t<Fn>, Storage<Key, Out>>{std::move(fn)};
}

}