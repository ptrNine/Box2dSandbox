#pragma once

#include <flat_hash_map.hpp>
#include <vector>

template <typename K, typename V>
class IndexedHashStorage {
    static_assert(!std::is_integral_v<K>, "Key can't be integer");
public:
    bool emplace_back(const K& key, const V& value) {
        auto found = _hmap.find(key);

        if (found == _hmap.end()) {
            _vec.push_back(key);
            _hmap.emplace(_vec.back(), ValueT(value, _vec.size() - 1));
            return true;
        }
        else {
            found->second.value = value;
            return false;
        }
    }

    void pop_back() {
        auto& key = _vec.back();
        _hmap.erase(key);
        _vec.pop_back();
    }

    std::optional<size_t> index_of(const K& key) const {
        auto found = _hmap.find(key);
        return found != _hmap.end() ? found->second.index : std::nullopt;
    }

    std::optional<K> key_of(size_t index) const {
        return index < _vec.size() ? _vec[index] : std::nullopt;
    }

    bool erase(const K& key) {
        auto found = _hmap.find(key);

        if (found == _hmap.end())
            return false;

        size_t idx = found->second.index;
        _hmap.erase(found);
        _vec.erase(_vec.begin() + idx);

        recalc(idx, true);

        return true;
    }

    size_t size() const {
        return _vec.size();
    }

    auto operator[](size_t idx)       ->       V& { return _hmap[_vec[idx]]; }
    auto operator[](size_t idx) const -> const V& { return _hmap[_vec[idx]]; }

    auto operator[](const K& key)       ->       V& { return _hmap[key]; }
    auto operator[](const K& key) const -> const V& { return _hmap[key]; }

    auto at(size_t idx)       ->       V& { return _hmap.at(_vec.at(idx)); }
    auto at(size_t idx) const -> const V& { return _hmap.at(_vec.at(idx)); }

    auto at(const K& key)       ->       V& { return _hmap.at(key); }
    auto at(const K& key) const -> const V& { return _hmap.at(key); }

    bool lookup(const K& key) const {
        return _hmap.find(key) != _hmap.end();
    }

    template <typename F>
    auto foreach(F&& callback) const -> std::enable_if_t<scl::args_count_v<F> == 1 &&
                                        std::is_same_v<
                                                std::decay_t<scl::arg_type_of_t<F, 0>>,
                                                V>>
    {
        for (auto& k : _vec)
            callback(_hmap[k].value);
    }

    template <typename F>
    auto foreach(F&& callback) const -> std::enable_if_t<scl::args_count_v<F> == 1 &&
                                        std::is_same_v<
                                                std::decay_t<scl::arg_type_of_t<F, 0>>,
                                                K>>
    {
        for (auto& k : _vec)
            callback(k);
    }

    template <typename F>
    auto foreach(F&& callback) const -> std::enable_if_t<scl::args_count_v<F> == 2 &&
                                        std::is_same_v<
                                                std::decay_t<scl::arg_type_of_t<F, 0>>,
                                                K>>
    {
        for (auto& k : _vec)
            callback(k, _hmap[k].value);
    }

    template <typename F>
    auto foreach(F&& callback) const -> std::enable_if_t<scl::args_count_v<F> == 3>
    {
        size_t idx = 0;
        for (auto& k : _vec)
            callback(k, _hmap[k].value, idx++);
    }

    template <typename F>
    auto foreach(F&& callback) -> std::enable_if_t<scl::args_count_v<F> == 1 &&
                                  std::is_same_v<
                                          std::decay_t<scl::arg_type_of_t<F, 0>>,
                                          V>>
    {
        for (auto& k : _vec)
            callback(_hmap[k].value);
    }

    template <typename F>
    auto foreach(F&& callback) -> std::enable_if_t<scl::args_count_v<F> == 1 &&
                                  std::is_same_v<
                                          std::decay_t<scl::arg_type_of_t<F, 0>>,
                                          K>>
    {
        for (auto& k : _vec)
            callback(k);
    }

    template <typename F>
    auto foreach(F&& callback) -> std::enable_if_t<scl::args_count_v<F> == 2 &&
                                  std::is_same_v<
                                          std::decay_t<scl::arg_type_of_t<F, 0>>,
                                          K>>
    {
        for (auto& k : _vec)
            callback(k, _hmap[k].value);
    }

    template <typename F>
    auto foreach(F&& callback) -> std::enable_if_t<scl::args_count_v<F> == 3>
    {
        size_t idx = 0;
        for (auto& k : _vec)
            callback(k, _hmap[k].value, idx++);
    }

    template <typename F>
    auto unordered_foreach(F&& callback) const -> std::enable_if_t<scl::args_count_v<F> == 1 &&
                                                  std::is_same_v<
                                                          std::decay_t<scl::arg_type_of_t<F, 0>>,
                                                          V>>
    {
        for (auto& p : _hmap)
            callback(p.second.value);
    }

    template <typename F>
    auto unordered_foreach(F&& callback) const -> std::enable_if_t<scl::args_count_v<F> == 2 &&
                                                  std::is_same_v<
                                                          std::decay_t<scl::arg_type_of_t<F, 0>>,
                                                          K>>
    {
        for (auto& p : _hmap)
            callback(p.first, p.second.value);
    }

    template <typename F>
    auto unordered_foreach(F&& callback) -> std::enable_if_t<scl::args_count_v<F> == 1 &&
                                            std::is_same_v<
                                                    std::decay_t<scl::arg_type_of_t<F, 0>>,
                                                    V>>
    {
        for (auto& p : _hmap)
            callback(p.second.value);
    }

    template <typename F>
    auto unordered_foreach(F&& callback) -> std::enable_if_t<scl::args_count_v<F> == 2 &&
                                            std::is_same_v<
                                                    std::decay_t<scl::arg_type_of_t<F, 0>>,
                                                    K>>
    {
        for (auto& p : _hmap)
            callback(p.first, p.second.value);
    }

protected:
    void recalc(size_t start, bool subtraction) {
        for (; start < _vec.size(); ++start)
            subtraction ? --_hmap[_vec[start]].index :
                          ++_hmap[_vec[start]].index;
    }

protected:
    struct ValueT {
        ValueT(const V& val, size_t ind): value(val), index(ind) {}
        ValueT() = default;

        V      value;
        size_t index;
    };

    ska::flat_hash_map<K, ValueT> _hmap;
    std::vector<K> _vec;
};