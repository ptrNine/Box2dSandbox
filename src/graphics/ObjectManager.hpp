#pragma once

#include <functional>
#include <flat_hash_map.hpp>

template <typename ObjectT, typename AttribT = void, class Enable = void>
class ObjectManager;


// ObjectManager with no attributes

#include <iostream>
template <typename ObjectT, typename AttribT>
class ObjectManager <ObjectT, AttribT, std::enable_if_t<std::is_same_v<AttribT, void>>> {
public:
    ObjectManager           (const ObjectManager<ObjectT, AttribT>&) = delete;
    ObjectManager& operator=(const ObjectManager<ObjectT, AttribT>&) = delete;

    ObjectManager() = default;

    ~ObjectManager() {
        std::cout << "Delete!" << std::endl;
        for (auto& p : _storage)
            delete p;
    }

    template <typename T, typename ... Ts>
    T* create(Ts &&... args) {
        T* obj = new T(args...);
        _storage.emplace(obj);
        return obj;
    }

    template <typename T>
    void remove(T *&object) {
        delete object;
        _storage.erase(object);
        object = nullptr;
    }

    template <typename T>
    T* push(T* obj) {
        _storage.emplace(obj);
        return obj;
    }

    template <typename T>
    void pop(T* obj) {
        _storage.erase(obj);
    }

    auto begin()       { return _storage.begin(); }
    auto begin() const { return _storage.begin(); }
    auto end  ()       { return _storage.end();   }
    auto end  () const { return _storage.end();   }

    void foreach(const std::function<void(ObjectT &)> &callback) {
        for (auto& obj : _storage)
            callback(*obj);
    }

private:
    ska::flat_hash_set<ObjectT*> _storage;
};


// ObjectManager with attributes

template <typename ObjectT, typename AttribT>
class ObjectManager <ObjectT, AttribT, std::enable_if_t<!std::is_same_v<AttribT, void>>> {
public:
    ObjectManager           (const ObjectManager<ObjectT, AttribT>&) = delete;
    ObjectManager& operator=(const ObjectManager<ObjectT, AttribT>&) = delete;

    ObjectManager() = default;

    ~ObjectManager() {
        for (auto& p : _storage)
            delete p.first;
    }

    template <typename T, typename ... Ts>
    T* create(Ts &&... args) {
        T* obj = new T(args...);
        _storage.emplace(obj, AttribT());
        return obj;
    }

    template <typename T>
    void remove(T *&object) {
        delete object;
        _storage.erase(object);
        object = nullptr;
    }

    template <typename T>
    T* push(T* obj, const AttribT& attribs) {
        _storage.emplace(obj, attribs);
        return obj;
    }

    template <typename T>
    void pop(T* obj) {
        _storage.erase(obj);
    }

    auto begin()       { return _storage.begin(); }
    auto begin() const { return _storage.begin(); }
    auto end  ()       { return _storage.end();   }
    auto end  () const { return _storage.end();   }

    void iterateAll(const std::function<void(ObjectT&, AttribT&)>& callback) {
        for (auto& obj : _storage)
            callback(*(obj.first), obj.second);
    }

private:
    ska::flat_hash_map<ObjectT*, AttribT> _storage;
};
