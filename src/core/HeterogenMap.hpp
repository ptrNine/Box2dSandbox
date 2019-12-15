#include <any>
#include <flat_hash_map.hpp>
#include <unordered_map>

#define HCASTER_REF(name, caster) auto& name = caster(#name)
#define HMAP_GET(type, name, heterogen_map) \
type name = heterogen_map.cast<type>(#name)

template <typename K, typename H = std::hash<K>, typename E = std::equal_to<K>, typename A = std::allocator<std::pair<K, std::any>>>
class HeterogenMap;


template <typename T, typename K, typename H, typename E, typename A>
class HeterogenMapCaster {
public:
    explicit HeterogenMapCaster(HeterogenMap<K>& hmap): _map(hmap) {}

    T& operator()(const K& key) {
        return _map.template cast<T>(key);
    }

    const T& operator()(const K& key) const {
        return _map.template cast<T>(key);
    }

private:
    class HeterogenMap<K, H, E, A>& _map;
};


template <typename K, typename H, typename E, typename A>
class HeterogenMap : public ska::flat_hash_map<K, std::any, H, E, A> {
public:
    template <typename T>
    auto get_caster() {
        return HeterogenMapCaster<T, K, H, E, A>(*this);
    }

    template <typename T>
    T& cast(const K& key) {
        return std::any_cast<T&>(this->at(key));
    }

    template <typename T>
    const T& cast(const K& key) const {
        return std::any_cast<T&>(this->at(key));
    }

    template <typename T>
    bool is_type(const K& key) const {
        return typeid(T) == this->at(key).type();
    }

    template <typename V, typename HH, typename EE, typename AA>
    HeterogenMap& operator=(const ska::flat_hash_map<K, V, HH, EE, AA>& hmap) {
        for (auto& pair : hmap)
            this->emplace(pair.first, pair.second);
        return *this;
    }

    template <typename V, typename HH, typename EE, typename AA>
    HeterogenMap& operator=(const std::unordered_map<K, V, HH, EE, AA>& hmap) {
        for (auto& pair : hmap)
            this->emplace(pair.first, pair.second);
        return *this;
    }

    bool has(const K& key) const {
        return this->find(key) != this->end();
    }
};