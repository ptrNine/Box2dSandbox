#pragma once

template <typename BaseType>
class DerivedNamedObjectManager {
public:
    template <typename T, typename... ArgsT>
    auto create(const std::string& name, ArgsT&&... args) -> typename std::weak_ptr<T> {
        auto ptr = std::make_shared<T>(args...);
        _data.emplace(name, ptr);
        return std::weak_ptr(ptr);
    }

    void erase(const std::string& name) {
        _data.erase(name);
    }

    auto get(const std::string& name) const -> std::weak_ptr<BaseType> {
        auto found = _data.find(name);

        if (found != _data.end())
            return found->second;
        else
            return {};
    }

    template <typename DerivedT>
    auto cast_get(const std::string& name) const -> std::weak_ptr<DerivedT> {
        auto found = _data.find(name);

        if (found != _data.end()) {
            auto casted = std::dynamic_pointer_cast<DerivedT>(found->second);
            return casted;
        } else
            return {};
    }

    auto& data() {
        return _data;
    }

    auto& data() const {
        return _data;
    }

protected:
    ska::flat_hash_map<std::string, std::shared_ptr<BaseType>> _data;
};

template <typename BaseType>
class DerivedObjectManager {
public:
    template <typename T, typename... ArgsT>
    auto create(ArgsT&&... args) -> typename std::weak_ptr<T> {
        auto ptr = std::make_shared<T>(args...);
        _data.emplace(ptr);
        return std::weak_ptr(ptr);
    }

    template <typename T>
    void erase(const std::shared_ptr<T>& obj) {
        _data.erase(obj);
    }

    auto& data() {
        return _data;
    }

protected:
    ska::flat_hash_set<std::shared_ptr<BaseType>> _data;
};