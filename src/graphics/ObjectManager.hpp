#pragma once

#include <functional>
//#include <flat_hash_map.hpp>
#include <unordered_map>
#include <unordered_set>

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
    ObjectManager(const std::string_view& name) : _name(name) {}

    ~ObjectManager() {
        for (auto& p : _storage)
            delete p;

        std::cout << "Destroy [" << _name << "]" << std::endl;
    }

    template <typename T, typename ... Ts>
    T* create(Ts &&... args) {
        T* obj = new T(args...);
        _storage.emplace(obj);
        return obj;
    }

    template <typename T>
    void remove(T* object) {
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

    size_t size() { return _storage.size(); }

    auto begin()       { return _storage.begin(); }
    auto begin() const { return _storage.begin(); }
    auto end  ()       { return _storage.end();   }
    auto end  () const { return _storage.end();   }

    void foreach(const std::function<void(ObjectT &)> &callback) {
        for (auto& obj : _storage)
            callback(*obj);
    }

private:
    //ska::flat_hash_set<ObjectT*> _storage;
    std::unordered_set<ObjectT*> _storage;
    std::string _name;
};


// ObjectManager with attributes

template <typename ObjectT, typename AttribT>
class ObjectManager <ObjectT, AttribT, std::enable_if_t<!std::is_same_v<AttribT, void>>> {
public:
    ObjectManager           (const ObjectManager<ObjectT, AttribT>&) = delete;
    ObjectManager& operator=(const ObjectManager<ObjectT, AttribT>&) = delete;

    ObjectManager() = default;
    ObjectManager(const std::string_view& name) : _name(name) {}

    ~ObjectManager() {
        for (auto& p : _storage)
            delete p.first;

        std::cout << "Destroy [" << _name << "]" << std::endl;
    }

    template <typename T, typename FirstT, typename ... Ts>
    auto create(const FirstT& first, Ts &&... args) -> std::enable_if_t<std::is_same_v<FirstT, AttribT>, T*> {
        T* obj = new T(args...);
        _storage.emplace(obj, first);
        return obj;
    }

    template <typename T, typename FirstT, typename ... Ts>
    auto create(const FirstT& first, Ts &&... args) -> std::enable_if_t<!std::is_same_v<FirstT, AttribT>, T*> {
        T* obj = new T(first, args...);
        _storage.emplace(obj, AttribT());
        return obj;
    }

    template <typename T>
    auto create() {
        T* obj = new T;
        _storage.emplace(obj, AttribT());
        return obj;
    }

    template <typename T, typename ... Ts>
    T* create(const AttribT& attrib, Ts &&... args) {
        T* obj = new T(args...);
        _storage.emplace(obj, attrib);
        return obj;
    }

    template <typename T>
    void remove(T* object) {
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

    size_t size() { return _storage.size(); }

    auto begin()       { return _storage.begin(); }
    auto begin() const { return _storage.begin(); }
    auto end  ()       { return _storage.end();   }
    auto end  () const { return _storage.end();   }

    void foreach(const std::function<void(ObjectT&, AttribT&)>& callback) {
        for (auto& obj : _storage)
            callback(*(obj.first), obj.second);
    }

private:
    //ska::flat_hash_map<ObjectT*, AttribT> _storage;
    std::unordered_map<ObjectT*, AttribT> _storage;
    std::string _name;
};


#include <libcuckoo/cuckoohash_map.hh>


template <typename ObjectT, typename AttribT = void, class Enable = void>
class TSObjectManager;

template <typename ObjectT, typename AttribT>
class TSObjectManager <ObjectT, AttribT, std::enable_if_t<std::is_same_v<AttribT, void>>> {
public:
    TSObjectManager           (const ObjectManager<ObjectT, AttribT>&) = delete;
    TSObjectManager& operator=(const ObjectManager<ObjectT, AttribT>&) = delete;

    TSObjectManager() = default;
    TSObjectManager(const std::string_view& name) : _name(name) {}

    ~TSObjectManager() {
        auto locked = _storage.lock_table();

        for (auto kv : locked)
            delete kv.first;

        std::cout << "Destroy [" << _name << "]" << std::endl;
    }

    template <typename T, typename ... Ts>
    T* create(Ts &&... args) {
        T* obj = new T(args...);
        _storage.insert(obj, 0);
        return obj;
    }

    template <typename T>
    void remove(T* object) {
        delete object;
        _storage.erase(object);
        object = nullptr;
    }

    template <typename T>
    T* push(T* obj) {
        _storage.insert(obj, 0);
        return obj;
    }

    template <typename T>
    void pop(T* obj) {
        _storage.erase(obj);
    }

    size_t size() { return _storage.size(); }

    void foreach(const std::function<void(ObjectT&)> &callback) {
        auto locked = _storage.lock_table();

        for (auto& obj : locked)
            callback(*(obj.first));
    }

private:
    cuckoohash_map<ObjectT*, size_t> _storage;
    std::string _name;
};


template <typename ObjectT, typename AttribT>
class TSObjectManager <ObjectT, AttribT, std::enable_if_t<!std::is_same_v<AttribT, void>>> {
public:
    TSObjectManager           (const ObjectManager<ObjectT, AttribT>&) = delete;
    TSObjectManager& operator=(const ObjectManager<ObjectT, AttribT>&) = delete;

    TSObjectManager() = default;
    TSObjectManager(const std::string_view& name) : _name(name) {}

    ~TSObjectManager() {
        auto locked = _storage.lock_table();

        for (auto kv : locked)
            delete kv.first;

        std::cout << "Destroy [" << _name << "]" << std::endl;
    }

    template <typename T, typename FirstT, typename ... Ts>
    auto create(const FirstT& first, Ts &&... args) -> std::enable_if_t<std::is_same_v<FirstT, AttribT>, T*> {
        T* obj = new T(args...);
        _storage.insert(obj, first);
        return obj;
    }

    template <typename T, typename FirstT, typename ... Ts>
    auto create(const FirstT& first, Ts &&... args) -> std::enable_if_t<!std::is_same_v<FirstT, AttribT>, T*> {
        T* obj = new T(first, args...);
        _storage.insert(obj, AttribT());
        return obj;
    }

    template <typename T>
    auto create() {
        T* obj = new T;
        _storage.insert(obj, AttribT());
        return obj;
    }

    template <typename T, typename ... Ts>
    T* create(const AttribT& attrib, Ts &&... args) {
        T* obj = new T(args...);
        _storage.insert(obj, attrib);
        return obj;
    }

    template <typename T>
    void remove(T* object) {
        delete object;
        _storage.erase(object);
        object = nullptr;
    }

    template <typename T>
    T* push(T* obj, const AttribT& attribs) {
        _storage.insert(obj, attribs);
        return obj;
    }

    template <typename T>
    void pop(T* obj) {
        _storage.erase(obj);
    }

    size_t size() { return _storage.size(); }

    void foreach(const std::function<void(ObjectT &, AttribT &)> &callback) {
        auto locked = _storage.lock_table();

        for (auto& obj : locked)
            callback(*(obj.first), obj.second);
    }

private:
    cuckoohash_map<ObjectT*, AttribT> _storage;
    std::string _name;
};